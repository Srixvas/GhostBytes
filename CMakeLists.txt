
cmake_minimum_required(VERSION 3.20)

project(GhostByte
    VERSION 0.1.0
    DESCRIPTION "Workload-Aware Adaptive Memory Reclamation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ghostbyte
    src/main.cpp
)

target_include_directories(ghostbyte
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(ghostbyte PRIVATE /W4)
else()
    target_compile_options(ghostbyte PRIVATE -Wall -Wextra -Wpedantic)
endif()
