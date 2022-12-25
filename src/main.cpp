#include <iostream>
#include <fmt/format.h>
#include "so_module.h"
int main(int argc, const char **argv) {
    try {
        auto lib = SharedLibraryModule("/home/marco/Scrivania/hobby/justagameloader/libs/chocolate-doom/chocolate-doom.aarch64");

        if ( const auto sym = lib.get_symbol("malloc")) {
            std::cout << fmt::format("{:16X} -> {}\n", sym.value(), "malloc");
        }
        if ( const auto sym = lib.get_symbol("JNI_OnLoad")) {
            std::cout << fmt::format("{:16X} -> {}\n", sym.value(), "JNI_OnLoad");
        }

        //std::cout << lib.get_name() << std::endl;
    }
    catch( std::runtime_error error ) {
        
        std::cerr << error.what() << std::endl;
    }
}