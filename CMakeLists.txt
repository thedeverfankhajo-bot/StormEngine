cmake_minimum_required(VERSION 3.20)
project(StormEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STORM_BUILD_TESTS "Build StormEngine tests" ON)

add_library(storm_core
    engine/core/src/Engine.cpp
)

target_include_directories(storm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/math/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/ecs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/render/include
)

target_compile_features(storm_core PUBLIC cxx_std_20)

add_executable(storm_sandbox
    examples/sandbox/main.cpp
)
target_link_libraries(storm_sandbox PRIVATE storm_core)

if(STORM_BUILD_TESTS)
    enable_testing()

    add_executable(storm_core_tests tests/core_tests.cpp)
    target_link_libraries(storm_core_tests PRIVATE storm_core)
    add_test(NAME storm_core_tests COMMAND storm_core_tests)

    add_executable(storm_ecs_tests tests/ecs_tests.cpp)
    target_link_libraries(storm_ecs_tests PRIVATE storm_core)
    add_test(NAME storm_ecs_tests COMMAND storm_ecs_tests)

    add_executable(storm_game_loop_tests tests/game_loop_tests.cpp)
    target_link_libraries(storm_game_loop_tests PRIVATE storm_core)
    add_test(NAME storm_game_loop_tests COMMAND storm_game_loop_tests)

    add_executable(storm_scheduler_tests tests/scheduler_tests.cpp)
    target_link_libraries(storm_scheduler_tests PRIVATE storm_core)
    add_test(NAME storm_scheduler_tests COMMAND storm_scheduler_tests)

    add_executable(storm_math_tests tests/math_tests.cpp)
    target_link_libraries(storm_math_tests PRIVATE storm_core)
    add_test(NAME storm_math_tests COMMAND storm_math_tests)

    add_executable(storm_camera_tests tests/camera_tests.cpp)
    target_link_libraries(storm_camera_tests PRIVATE storm_core)
    add_test(NAME storm_camera_tests COMMAND storm_camera_tests)

    add_executable(storm_world_tests tests/world_tests.cpp)
    target_link_libraries(storm_world_tests PRIVATE storm_core)
    add_test(NAME storm_world_tests COMMAND storm_world_tests)
endif()
