cmake_minimum_required(VERSION 3.16)

project(RISCV_Emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
    src/main.cpp
    src/ram.cpp
    src/cache_abstract.cpp
    src/cache_lru.cpp
    src/cache_bplru.cpp
    src/processor.cpp
)

add_executable(emulator ${SOURCE_FILES})

target_include_directories(emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
