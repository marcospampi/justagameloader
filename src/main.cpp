#include <iostream>
#include "types.hpp"
#include "thread.hpp"
#include "so_module.h"
using VAddr = u64;

int main(int argc, const char **argv, const char **envv) {
    if ( argc < 2 ) return 1;
    const auto elf_file = argv[1];
    try {
        auto lib = Elf(elf_file);

        for ( const auto &entry: *get_thunks() ) {
            if ( entry.name ) {
                lib.hook_symbol( entry.name.value(), entry.address );
            }
        }

        lib.hook_symbol ( "__stack_chk_guard", reinterpret_cast<uintptr_t>(&main) );
        lib.hook_symbol ( "stdout", reinterpret_cast<uintptr_t>(&stderr) );
        lib.hook_symbol ( "stderr", reinterpret_cast<uintptr_t>(&stderr) );

        auto thread = create_thread(4096*16);
        auto main_syn = lib.get_symbol("main").value();

        // const char *fake_argv[] = {"./chocolate-doom", "-nosound", "-nosfx", "-nomusic", "-window"};
        const std::vector<const char*> fake_argv = {
            "./chocolate-doom", "-nosound"
        };
        auto result = thread->jit->VMCall<int>(main_syn, fake_argv.size(), (u64)fake_argv.data());
     
    }
    catch( std::runtime_error error ) {
        
        std::cerr << error.what() << std::endl;
    }
}