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
    src/config.cpp
    src/system_monitor.cpp
    src/pressure_analyzer.cpp
    src/policy_engine.cpp
    src/reclamation_controller.cpp
    src/logger.cpp
    src/feedback_analyzer.cpp
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


enable_testing()

add_executable(ghostbyte_tests
    tests/test_pressure_policy.cpp
    src/config.cpp
    src/pressure_analyzer.cpp
    src/policy_engine.cpp
    src/reclamation_controller.cpp
    src/feedback_analyzer.cpp
)

target_include_directories(ghostbyte_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME ghostbyte_tests COMMAND ghostbyte_tests)
# --- Integration Test Runner ---
add_executable(ghostbyte_reclaim_runner
    tests/run_real_reclaim.cpp
    src/reclamation_controller.cpp
    src/policy_engine.cpp
)
target_include_directories(ghostbyte_reclaim_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
