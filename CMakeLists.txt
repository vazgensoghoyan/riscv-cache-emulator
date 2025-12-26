cmake_minimum_required(VERSION 3.20)

project(
    riscv-cache-simulator
    VERSION 1.0
    DESCRIPTION "RV32I/M RISC-V CPU emulator with configurable LRU and bpLRU cache policies"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

file(GLOB_RECURSE RISC_V_ALL_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB RISC_V_MAIN
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

list(REMOVE_ITEM RISC_V_ALL_SOURCES ${RISC_V_MAIN})

add_library(riscv_core STATIC)

target_sources(riscv_core
    PRIVATE
        ${RISC_V_ALL_SOURCES}
)

target_include_directories(riscv_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(riscv_core
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wpedantic
)

add_executable(riscv-cache-sim
    ${RISC_V_MAIN}
)

target_link_libraries(riscv-cache-sim
    PRIVATE
        riscv_core
)

option(ENABLE_SANITIZERS "Enable address/UB sanitizers" OFF)

if(ENABLE_SANITIZERS)
    target_compile_options(riscv_core PRIVATE -fsanitize=address,undefined)
    target_link_options(riscv-cache-sim PRIVATE -fsanitize=address,undefined)
endif()
