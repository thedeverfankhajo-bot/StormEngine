cmake_minimum_required(VERSION 3.20)
project(StormEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STORM_BUILD_TESTS "Build StormEngine tests" ON)

add_library(storm_core
    engine/core/src/Engine.cpp
    engine/render/src/NullRenderDevice.cpp
)

target_include_directories(storm_core PUBLIC
    engine/core/include
    engine/math/include
    engine/ecs/include
    engine/render/include
)

add_executable(storm_sandbox examples/sandbox/main.cpp)
target_link_libraries(storm_sandbox PRIVATE storm_core)

if(STORM_BUILD_TESTS)
    enable_testing()

    function(storm_add_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE storm_core)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    storm_add_test(storm_core_tests tests/core_tests.cpp)
    storm_add_test(storm_ecs_tests tests/ecs_tests.cpp)
    storm_add_test(storm_game_loop_tests tests/game_loop_tests.cpp)
    storm_add_test(storm_scheduler_tests tests/scheduler_tests.cpp)
    storm_add_test(storm_math_tests tests/math_tests.cpp)
    storm_add_test(storm_camera_tests tests/camera_tests.cpp)
    storm_add_test(storm_world_tests tests/world_tests.cpp)
    storm_add_test(storm_render_tests tests/render_tests.cpp)
    storm_add_test(storm_render_device_tests tests/render_device_tests.cpp)
    storm_add_test(storm_mesh_tests tests/mesh_tests.cpp)
    storm_add_test(storm_render_resources_tests tests/render_resources_tests.cpp)
endif()
