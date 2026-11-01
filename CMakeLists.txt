cmake_minimum_required(VERSION 3.24)

project(StormEngine
    VERSION 0.1.0
    DESCRIPTION "A mobile-only 1D/2D/3D game engine foundation"
    LANGUAGES CXX
)

include(CTest)

option(STORMENGINE_BUILD_TESTS "Build host-side StormEngine tests" ${BUILD_TESTING})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(stormengine_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wformat=2
        )
    endif()
endfunction()

add_library(storm_foundation STATIC
    engine/foundation/src/logging.cpp
)
add_library(Storm::Foundation ALIAS storm_foundation)
target_include_directories(storm_foundation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/foundation/include>
)
target_compile_features(storm_foundation PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_foundation)

add_library(storm_core STATIC
    engine/core/src/engine.cpp
)
add_library(Storm::Core ALIAS storm_core)
target_include_directories(storm_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include>
)
target_link_libraries(storm_core PUBLIC Storm::Foundation)
target_compile_features(storm_core PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_core)

add_library(storm_platform STATIC
    engine/platform/src/lifecycle.cpp
    engine/platform/src/lifecycle_event_queue.cpp
    engine/platform/src/surface_lifecycle.cpp
)
add_library(Storm::Platform ALIAS storm_platform)
target_include_directories(storm_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/platform/include>
)
target_link_libraries(storm_platform PUBLIC Storm::Foundation)
target_compile_features(storm_platform PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_platform)

if(STORMENGINE_BUILD_TESTS)
    if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(STATUS "StormEngine host tests are disabled for mobile target builds")
    else()
        add_executable(stormengine_foundation_tests
            tests/foundation_tests.cpp
        )
        target_link_libraries(stormengine_foundation_tests PRIVATE Storm::Core Storm::Foundation Storm::Platform)
        target_compile_features(stormengine_foundation_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_foundation_tests)
        add_test(NAME stormengine_foundation_tests COMMAND stormengine_foundation_tests)

        add_executable(stormengine_platform_surface_tests
            tests/platform_surface_tests.cpp
        )
        target_link_libraries(stormengine_platform_surface_tests PRIVATE Storm::Platform)
        target_compile_features(stormengine_platform_surface_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_platform_surface_tests)
        add_test(NAME stormengine_platform_surface_tests COMMAND stormengine_platform_surface_tests)
    endif()
endif()
