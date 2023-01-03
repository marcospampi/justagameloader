#include <elf.h>

#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <cstring>
#include <iostream>
#include <vector>

#include "so_module.h"




auto is_elf_valid(Elf64_Ehdr &header) -> bool {
    return  header.e_ident[EI_MAG0] == 0x7f
        &&  header.e_ident[EI_MAG1] == 'E'
        &&  header.e_ident[EI_MAG2] == 'L'
        &&  header.e_ident[EI_MAG3] == 'F'
        &&  header.e_ident[EI_CLASS] == ELFCLASS64
        &&  header.e_machine == EM_AARCH64;
}


auto get_section_headers(Elf64_Ehdr &header, std::ifstream &file) -> std::vector<Elf64_Shdr> {
    std::vector<Elf64_Shdr> data{};
    data.resize(header.e_shnum);
    file.seekg(header.e_shoff);
    file.read( reinterpret_cast<char*>(data.data()), sizeof(Elf64_Shdr[header.e_shnum]));
    

    return data;
}

auto get_section_names(Elf64_Ehdr &header, std::vector<Elf64_Shdr> &sections, std::ifstream &file) -> std::unique_ptr<char[]> {

    auto &section = sections[header.e_shstrndx];
    file.seekg(section.sh_offset);
    std::unique_ptr<char[]> section_names(new char[section.sh_size]);

    file.read( section_names.get(), section.sh_size);
    

    return section_names;
}

auto load_segments(std::span<Elf64_Phdr> headers, std::ifstream &file ) -> std::unique_ptr<u8[]> {
    size_t allocation_size = 0;
    size_t align = 0;
    bool header_contained = false;
    for ( auto &head : headers ) {
        if ( head.p_type == PT_LOAD ) {
            //size_t virt_offs = head.p_vaddr & ~(head.p_align - 1);
            size_t virt_size = (head.p_memsz + (head.p_align - 1)) & ~(head.p_align - 1);
            allocation_size += virt_size;
            if ( align < head.p_align ) {
                align = head.p_align;
            }
            if ( head.p_offset == 0 ) {
                header_contained = true;
            }
        }
    }

    // header must be contained
    if ( header_contained == false ) {
        throw std::runtime_error("Header is not contained into allocated data!");
    }
    // make it managed
    std::unique_ptr<u8[]> allocation {new (std::align_val_t(align)) u8[allocation_size]};
    
    // clean the vector
    std::memset(allocation.get(), 0, allocation_size);

    uintptr_t alloc_addr = reinterpret_cast<uintptr_t>(allocation.get());

    for ( auto &head : headers ) {
        if (head.p_type == PT_LOAD) {
            head.p_vaddr += alloc_addr;

            file.seekg(head.p_offset, std::ios::beg);
            file.read(reinterpret_cast<char *>(head.p_vaddr), head.p_filesz);
            if (file.bad()) {
                throw std::runtime_error("diocane");
            }
        }
    }

    return std::move(allocation);

}

auto relocate(u8 *allocation, Elf64_Ehdr &header, std::vector<Elf64_Shdr> &sections, const char *section_names) -> auto {

    std::span<Elf64_Dyn> dynamic{};
    const char *dynstr{nullptr};
    Elf64_Shdr *dynsym_section;
    std::span<Elf64_Sym> dynsym{};
    std::span<Elf64_Rela> reladyn{};
    std::span<Elf64_Rela> relaplt{};
    std::span<uintptr_t> init_array{};



    for ( auto &section: sections ) {
        const char *name = &section_names[section.sh_name];
        const auto addr = section.sh_offset;

        const auto count = section.sh_entsize == 0 
            ? section.sh_size
            : section.sh_size / (section.sh_entsize );
        if ( section.sh_type == SHT_SYMTAB ) {
            std::cout << name << std::endl;
        }
        if ( name == std::string_view(".dynamic") ) {
            dynamic = std::span(reinterpret_cast<Elf64_Dyn*>(&allocation[addr]), count);
        } 
        else if ( name == std::string_view(".dynstr") ) {
            dynstr = reinterpret_cast<char*>(&allocation[addr]);
        } 
        else if ( name == std::string_view(".dynsym") ) {
            dynsym_section = &section;
            dynsym = std::span(reinterpret_cast<Elf64_Sym*>(&allocation[addr]), count);
        } 
        else if ( name == std::string_view(".rela.dyn") ) {
            reladyn = std::span(reinterpret_cast<Elf64_Rela*>(&allocation[addr]), count);
        } 
        else if ( name == std::string_view(".rela.plt") ) {
            relaplt = std::span(reinterpret_cast<Elf64_Rela*>(&allocation[addr]), count);
        } 
        else if ( name == std::string_view(".init_array") ) {
            init_array = std::span(reinterpret_cast<uintptr_t*>(&allocation[addr]), count);
        }
    }

    
    Elf::symbol_table symbols;

    const auto base = reinterpret_cast<uintptr_t>(allocation);
    auto relocate_symbols = [&](std::span<Elf64_Rela> &entries) {
        for ( auto &entry : entries ) {
            auto &symbol = dynsym[ELF64_R_SYM(entry.r_info)];
            auto type = ELF64_R_TYPE(entry.r_info);
            uintptr_t *ptr = reinterpret_cast<uintptr_t*>(allocation + entry.r_offset);
            const std::string name = &dynstr[symbol.st_name];

            switch ( type )
            {
                case R_AARCH64_RELATIVE:{
                    *ptr = base + entry.r_addend;
                    break;
                }
                case R_AARCH64_ABS64:{
                        *ptr = base + symbol.st_value + entry.r_addend;
                    break;
                }
                case R_AARCH64_GLOB_DAT:
                case R_AARCH64_JUMP_SLOT:{
                    if (symbol.st_shndx != SHN_UNDEF) {
                        *ptr = base + symbol.st_value + entry.r_addend;
                    }
                    break;
                }
                default:
                    // who cares lmao
                    std::cerr << fmt::format("Error unknown relocation type: {}\n", type);
                    break;
            }

            if ( name.length() > 0 ){
                symbols[name] = {
                    .address = reinterpret_cast<uintptr_t>(ptr),
                    .is_import = true
                };
            }

        }
    };

    relocate_symbols(reladyn);
    relocate_symbols(relaplt);

    for ( auto &symbol: dynsym ) {

        const std::string name = &dynstr[symbol.st_name];
        uintptr_t ptr = base + symbol.st_value;
        if ( name == "main" ) {
            std::cout << name << std::endl;
        }
        if ( name.length() > 0 && !symbols.contains(name) ){
            symbols[name] = {
                .address = reinterpret_cast<uintptr_t>(ptr),
                .is_import = false
            };
        }
    }

    if ( header.e_entry ) {
        symbols["_start"] = {
            .address = base + header.e_entry,
            .is_import = false
        };
    }
    
    for ( auto &section : sections ) {
        const char *name = &section_names[section.sh_name];
        if ( name == std::string_view(".text") && !symbols.contains("main") ) {
            symbols["main"] = {
                .address = base + section.sh_offset,
                .is_import = false
            };

        }
    }


    return std::make_tuple(symbols, init_array);


}
Elf::~Elf() {}
Elf::Elf(const std::filesystem::path &library) {
    #define CHECK_FILE() if ( file.bad() ) throw std::runtime_error(fmt::format("Failed to open file: {}", library.c_str()));

    std::ifstream file(library);

    CHECK_FILE();

    Elf64_Ehdr header;
    file.read(reinterpret_cast<char*>(&header), sizeof(Elf64_Ehdr));

    CHECK_FILE();
    if (!is_elf_valid(header)) {
        throw std::runtime_error("Invalid file");
    }


    const auto program_headers_count = header.e_phnum;
    Elf64_Phdr program_headers[header.e_phnum];
    file.read(reinterpret_cast<char*>(program_headers), sizeof(program_headers));

    CHECK_FILE();

    this->allocation = load_segments(std::span(program_headers, program_headers_count), file);
    auto sections = get_section_headers(header, file);
    auto section_names = get_section_names(header, sections, file);
    auto [ symbols, init_array ] = relocate(allocation.get(), header, sections, section_names.get());
    this->symbols = symbols;
    this->init_array = init_array;
}

const Elf::symbol_table &Elf::get_symbol_table() const {
    return symbols;
}
std::optional<uintptr_t> Elf::get_symbol(const std::string &sym) const {
    if ( symbols.contains(sym) ) {
        const auto &symbol = symbols.at(sym);
        if ( symbol.is_import ) {
            return *reinterpret_cast<uintptr_t*>(symbols.at(sym).address);
        }
        else {
            return symbols.at(sym).address;
        }
    }
    else {
        return std::nullopt;
    }
}
void Elf::hook_symbol(const std::string &sym, uintptr_t ptr) {
    if ( symbols.contains(sym) ) {
        const auto &symbol = symbols.at(sym);
        if ( symbol.is_import ) {
            return patch_symbol(symbol.address, ptr);
        }
        else {
            return patch_aarch64(symbol.address, ptr);
        }
    }
}
void Elf::patch_symbol(uintptr_t addr, uintptr_t with) {
    *reinterpret_cast<uintptr_t *>(addr) = with;
}
/// SAUCE: https://github.com/fgsfdsfgs/max_nx/blob/master/source/so_util.c#L68
/// thanks to fgsfdsfgs ( didn't copy paste ze name )
void Elf::patch_aarch64(uintptr_t addr, uintptr_t with) {
    uint32_t *hook = reinterpret_cast<uint32_t *>(addr);
    hook[0] = 0x58000051u; // LDR X17, #0x8
    hook[1] = 0xd61f0220u; // BR X17
    *reinterpret_cast<uintptr_t *>(hook + 2) = with;
}