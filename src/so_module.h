#pragma once
#include <memory>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <span>
#include <optional>
#include "types.hpp"

class Elf {
public:
    struct symbol_entry {
        uintptr_t address;
        bool is_import;
    };

    using symbol_table = std::unordered_map<std::string, symbol_entry>;

    Elf(const std::filesystem::path &library);

    const symbol_table &get_symbol_table() const;

    std::optional<uintptr_t> get_symbol(const std::string &sym) const;
    void hook_symbol(const std::string &sym, uintptr_t ptr); 

    ~Elf();
private:
    void patch_aarch64(uintptr_t addr, uintptr_t with);
    void patch_symbol(uintptr_t addr, uintptr_t with);

    std::unique_ptr<u8[]> allocation;
    std::span<uintptr_t> init_array{};
    symbol_table symbols;
};