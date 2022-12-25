cmake_minimum_required(VERSION 3.12)
project(dynarmic LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_subdirectory(external/dynarmic)

add_executable(
    loader
    src/main.cpp
    src/so_module.cpp
)
find_package(fmt)
target_include_directories(loader PRIVATE external/include external/dynarmic/include)
target_link_libraries(loader PRIVATE fmt)