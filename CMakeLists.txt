cmake_minimum_required(VERSION 3.12)
project(dynarmic LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DYNARMIC_FRONTENDS "A64")
add_subdirectory(external/dynarmic)


add_executable(
    loader
    src/main.cpp
    src/so_module.cpp
    src/helpers.cpp
    src/thunks.cpp
    src/thread.cpp
)
find_package(fmt)
find_package(SDL2)
find_package(SDL2_mixer)
find_package(SDL2_net)

target_include_directories(loader PRIVATE external/include external/dynarmic/include external/dynarmic/externals/mcl/include )
target_link_libraries(loader PRIVATE fmt dynarmic mcl SDL2 SDL2_mixer SDL2_net)