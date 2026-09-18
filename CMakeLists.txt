cmake_minimum_required(VERSION 3.20)
project(StormEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STORM_BUILD_TESTS "Build StormEngine tests" ON)
option(STORM_BUILD_GLES "Build OpenGL ES 3.x backend" OFF)
option(STORM_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

add_library(storm_core
    engine/core/src/Engine.cpp
    engine/render/src/NullRenderDevice.cpp
    engine/render/src/GlesContext.cpp
)

if(STORM_BUILD_GLES)
    target_sources(storm_core PRIVATE engine/render/src/GlesRenderDevice.cpp)
    if(ANDROID)
        target_link_libraries(storm_core PRIVATE GLESv3 EGL)
    endif()
endif()

target_include_directories(storm_core PUBLIC
    engine/core/include
    engine/math/include
    engine/ecs/include
    engine/render/include
    engine/physics/include
)

if(STORM_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(storm_core PRIVATE /W4)
    else()
        target_compile_options(storm_core PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(ANDROID)
    add_library(storm_android_smoke SHARED android/app/src/main/cpp/native_smoke.cpp)
    target_link_libraries(storm_android_smoke PRIVATE storm_core GLESv3 EGL)
endif()

if(NOT ANDROID)
    add_executable(storm_sandbox examples/sandbox/main.cpp)
    target_link_libraries(storm_sandbox PRIVATE storm_core)
endif()

if(STORM_BUILD_TESTS AND NOT ANDROID)
    enable_testing()

    function(storm_add_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE storm_core)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    storm_add_test(storm_core_tests tests/core_tests.cpp)
    storm_add_test(storm_resource_pool_tests tests/resource_pool_tests.cpp)
    storm_add_test(storm_ecs_tests tests/ecs_tests.cpp)
    storm_add_test(storm_game_loop_tests tests/game_loop_tests.cpp)
    storm_add_test(storm_scheduler_tests tests/scheduler_tests.cpp)
    storm_add_test(storm_math_tests tests/math_tests.cpp)
    storm_add_test(storm_dimensional_math_tests tests/dimensional_math_tests.cpp)
    storm_add_test(storm_camera_tests tests/camera_tests.cpp)
    storm_add_test(storm_camera1d_tests tests/camera1d_tests.cpp)
    storm_add_test(storm_camera2d_tests tests/camera2d_tests.cpp)
    storm_add_test(storm_camera3d_tests tests/camera3d_tests.cpp)
    storm_add_test(storm_world_tests tests/world_tests.cpp)
    storm_add_test(storm_render_tests tests/render_tests.cpp)
    storm_add_test(storm_render_device_tests tests/render_device_tests.cpp)
    storm_add_test(storm_render_queue_tests tests/render_queue_tests.cpp)
    storm_add_test(storm_render_state_cache_tests tests/render_state_cache_tests.cpp)
    storm_add_test(storm_mesh_tests tests/mesh_tests.cpp)
    storm_add_test(storm_render_resources_tests tests/render_resources_tests.cpp)
    storm_add_test(storm_gles_context_tests tests/gles_context_tests.cpp)
    storm_add_test(storm_physics2d_tests tests/physics2d_tests.cpp)
    storm_add_test(storm_sprite_batch_tests tests/sprite_batch_tests.cpp)
endif()
