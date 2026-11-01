cmake_minimum_required(VERSION 3.24)

project(StormEngine
    VERSION 0.1.0
    DESCRIPTION "A mobile-only 1D/2D/3D game engine foundation"
    LANGUAGES CXX
)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(CMAKE_OBJCXX_EXTENSIONS OFF)
endif()

include(CTest)

option(STORMENGINE_BUILD_TESTS "Build host-side StormEngine tests" ${BUILD_TESTING})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(stormengine_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:/W4>"
            "$<$<COMPILE_LANGUAGE:CXX>:/permissive->"
        )
    else()
        target_compile_options(${target_name} PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wconversion>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wshadow>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wformat=2>"
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
    engine/core/src/fixed_step_scheduler.cpp
    engine/core/src/game_loop.cpp
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
    engine/platform/src/android_lifecycle_adapter.cpp
    engine/platform/src/ios_lifecycle_adapter.cpp
)
add_library(Storm::Platform ALIAS storm_platform)
target_include_directories(storm_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/platform/include>
)
target_link_libraries(storm_platform PUBLIC Storm::Foundation)
target_compile_features(storm_platform PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_platform)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    find_library(STORMENGINE_UIKIT_FRAMEWORK UIKit REQUIRED)

    target_sources(storm_platform PRIVATE
        platforms/ios/scene_lifecycle_delegate.mm
    )
    target_include_directories(storm_platform
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/include>
    )
    target_link_libraries(storm_platform PUBLIC "${STORMENGINE_UIKIT_FRAMEWORK}")
    # The host application selects StormEngineSceneDelegate by class name in
    # its scene configuration; -ObjC loads the class object from this archive.
    target_link_options(storm_platform INTERFACE "-ObjC")
endif()

if(ANDROID)
    # The NDK's Native App Glue is C source shipped with the selected NDK.
    # Enabling C only for Android keeps the host/iOS project surface minimal.
    enable_language(C)

    if(NOT CMAKE_ANDROID_NDK)
        message(FATAL_ERROR "CMAKE_ANDROID_NDK is required for the Android NativeActivity target")
    endif()

    set(STORM_ANDROID_NATIVE_APP_GLUE_DIR
        "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
    if(NOT EXISTS "${STORM_ANDROID_NATIVE_APP_GLUE_DIR}/android_native_app_glue.h" OR
       NOT EXISTS "${STORM_ANDROID_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
        message(FATAL_ERROR
            "The selected Android NDK does not contain sources/android/native_app_glue")
    endif()

    add_library(storm_android_native_activity SHARED
        platforms/android/native_activity.cpp
        "${STORM_ANDROID_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
    )
    target_include_directories(storm_android_native_activity
        PRIVATE
            "${STORM_ANDROID_NATIVE_APP_GLUE_DIR}"
    )
    target_link_libraries(storm_android_native_activity
        PRIVATE
            Storm::Platform
            android
            log
    )
    target_compile_features(storm_android_native_activity PRIVATE cxx_std_20)
    stormengine_apply_warnings(storm_android_native_activity)
endif()

if(STORMENGINE_BUILD_TESTS)
    if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(STATUS "StormEngine host tests are disabled for mobile target builds")
    else()
        add_executable(stormengine_foundation_tests
            tests/foundation_tests.cpp
        )
        # Core and Platform expose Foundation transitively; linking it again
        # here duplicates the static library on some linkers.
        target_link_libraries(stormengine_foundation_tests PRIVATE Storm::Core Storm::Platform)
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

        add_executable(stormengine_android_lifecycle_tests
            tests/android_lifecycle_tests.cpp
        )
        target_link_libraries(stormengine_android_lifecycle_tests PRIVATE Storm::Platform)
        target_compile_features(stormengine_android_lifecycle_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_android_lifecycle_tests)
        add_test(NAME stormengine_android_lifecycle_tests COMMAND stormengine_android_lifecycle_tests)

        add_executable(stormengine_ios_lifecycle_tests
            tests/ios_lifecycle_tests.cpp
        )
        target_link_libraries(stormengine_ios_lifecycle_tests PRIVATE Storm::Platform)
        target_compile_features(stormengine_ios_lifecycle_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_ios_lifecycle_tests)
        add_test(NAME stormengine_ios_lifecycle_tests COMMAND stormengine_ios_lifecycle_tests)

        add_executable(stormengine_fixed_step_scheduler_tests
            tests/fixed_step_scheduler_tests.cpp
        )
        target_link_libraries(stormengine_fixed_step_scheduler_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_fixed_step_scheduler_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_fixed_step_scheduler_tests)
        add_test(NAME stormengine_fixed_step_scheduler_tests COMMAND stormengine_fixed_step_scheduler_tests)

        add_executable(stormengine_game_loop_tests
            tests/game_loop_tests.cpp
        )
        target_link_libraries(stormengine_game_loop_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_game_loop_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_game_loop_tests)
        add_test(NAME stormengine_game_loop_tests COMMAND stormengine_game_loop_tests)

        add_executable(stormengine_event_dispatcher_tests
            tests/event_dispatcher_tests.cpp
        )
        target_link_libraries(stormengine_event_dispatcher_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_event_dispatcher_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_event_dispatcher_tests)
        add_test(NAME stormengine_event_dispatcher_tests COMMAND stormengine_event_dispatcher_tests)
    endif()
endif()
