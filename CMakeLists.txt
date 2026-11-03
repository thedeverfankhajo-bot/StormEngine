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

add_library(storm_math STATIC
    engine/math/src/transform2d.cpp
    engine/math/src/camera2d.cpp
    engine/math/src/transform3d.cpp
)
add_library(Storm::Math ALIAS storm_math)
target_include_directories(storm_math
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/math/include>
)
target_link_libraries(storm_math PUBLIC Storm::Foundation)
target_compile_features(storm_math PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_math)

add_library(storm_physics STATIC
    engine/physics/src/one_d.cpp
)
add_library(Storm::Physics1D ALIAS storm_physics)
target_include_directories(storm_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics/include>
)
target_link_libraries(storm_physics PUBLIC Storm::Foundation)
target_compile_features(storm_physics PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_physics)

add_library(storm_physics2d STATIC
    engine/physics/src/two_d.cpp
)
add_library(Storm::Physics2D ALIAS storm_physics2d)
target_include_directories(storm_physics2d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics/include>
)
target_link_libraries(storm_physics2d PUBLIC Storm::Foundation)
target_compile_features(storm_physics2d PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_physics2d)

add_library(storm_input STATIC
    engine/input/src/touch_input.cpp
    engine/input/src/gesture_recognizer.cpp
    engine/input/src/input_action_map.cpp
)
add_library(Storm::Input ALIAS storm_input)
target_include_directories(storm_input
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/input/include>
)
target_link_libraries(storm_input PUBLIC Storm::Foundation)
target_compile_features(storm_input PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_input)

add_library(storm_core STATIC
    engine/core/src/engine.cpp
    engine/core/src/fixed_step_scheduler.cpp
    engine/core/src/game_loop.cpp
    engine/core/src/scene.cpp
    engine/core/src/transform_system.cpp
    engine/core/src/transform_system_2d.cpp
)
add_library(Storm::Core ALIAS storm_core)
target_include_directories(storm_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include>
)
target_link_libraries(storm_core PUBLIC Storm::Foundation Storm::Math)
target_compile_features(storm_core PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_core)

add_library(storm_serialization STATIC
    engine/serialization/src/binary_archive.cpp
    engine/serialization/src/scene_serializer.cpp
)
add_library(Storm::Serialization ALIAS storm_serialization)
target_include_directories(storm_serialization
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/serialization/include>
)
target_link_libraries(storm_serialization PUBLIC Storm::Core Storm::Math)
target_compile_features(storm_serialization PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_serialization)

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

add_library(storm_runtime STATIC
    engine/runtime/src/mobile_frame_driver.cpp
    engine/runtime/src/touch_input_game_loop_adapter.cpp
    engine/runtime/src/gesture_action_input_pipeline.cpp
)
add_library(Storm::Runtime ALIAS storm_runtime)
target_include_directories(storm_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/runtime/include>
)
target_link_libraries(storm_runtime PUBLIC Storm::Core Storm::Platform Storm::Input)
target_compile_features(storm_runtime PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_runtime)

# Optional bridge: Core and the standalone Physics1D target stay decoupled.
add_library(storm_physics_scene1d STATIC
    engine/physics/src/one_d_scene.cpp
)
add_library(Storm::PhysicsScene1D ALIAS storm_physics_scene1d)
target_include_directories(storm_physics_scene1d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics/include>
)
target_link_libraries(storm_physics_scene1d
    PUBLIC
        Storm::Physics1D
        Storm::Core
        Storm::Math
)
target_compile_features(storm_physics_scene1d PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_physics_scene1d)

# Optional bridge: Physics2D remains independent from Core and Scene.
add_library(storm_physics_scene2d STATIC
    engine/physics/src/two_d_scene.cpp
)
add_library(Storm::PhysicsScene2D ALIAS storm_physics_scene2d)
target_include_directories(storm_physics_scene2d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics/include>
)
target_link_libraries(storm_physics_scene2d
    PUBLIC
        Storm::Physics2D
        Storm::Core
        Storm::Math
)
target_compile_features(storm_physics_scene2d PUBLIC cxx_std_20)
stormengine_apply_warnings(storm_physics_scene2d)

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

    # Minimal iOS host application. This links the UIKit lifecycle adapter
    # but does not claim to implement a renderer or game runtime.
    add_executable(storm_ios_app MACOSX_BUNDLE
        platforms/ios/app/main.mm
        platforms/ios/app/app_delegate.mm
    )
    target_include_directories(storm_ios_app PRIVATE
        platforms/ios/include
    )
    target_link_libraries(storm_ios_app PRIVATE
        Storm::Runtime
        Storm::Serialization
        Storm::Physics1D
        Storm::Platform
        Storm::Input
    )
    set_target_properties(storm_ios_app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.stormengine.runtime"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.stormengine.runtime"
        MACOSX_BUNDLE_BUNDLE_NAME "StormEngine"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/app/Info.plist"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO"
    )
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
            Storm::Input
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

        add_executable(stormengine_mobile_frame_driver_tests
            tests/mobile_frame_driver_tests.cpp
        )
        target_link_libraries(stormengine_mobile_frame_driver_tests PRIVATE Storm::Runtime)
        target_compile_features(stormengine_mobile_frame_driver_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_mobile_frame_driver_tests)
        add_test(NAME stormengine_mobile_frame_driver_tests COMMAND stormengine_mobile_frame_driver_tests)

        add_executable(stormengine_event_dispatcher_tests
            tests/event_dispatcher_tests.cpp
        )
        target_link_libraries(stormengine_event_dispatcher_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_event_dispatcher_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_event_dispatcher_tests)
        add_test(NAME stormengine_event_dispatcher_tests COMMAND stormengine_event_dispatcher_tests)

        add_executable(stormengine_math_tests
            tests/math_tests.cpp
        )
        target_link_libraries(stormengine_math_tests PRIVATE Storm::Math)
        target_compile_features(stormengine_math_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_math_tests)
        add_test(NAME stormengine_math_tests COMMAND stormengine_math_tests)

        add_executable(stormengine_transform_system_tests
            tests/transform_system_tests.cpp
        )
        target_link_libraries(stormengine_transform_system_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_transform_system_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_transform_system_tests)
        add_test(NAME stormengine_transform_system_tests COMMAND stormengine_transform_system_tests)

        add_executable(stormengine_scene_tests
            tests/scene_tests.cpp
        )
        target_link_libraries(stormengine_scene_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_scene_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_scene_tests)
        add_test(NAME stormengine_scene_tests COMMAND stormengine_scene_tests)

        add_executable(stormengine_scene_serialization_tests
            tests/scene_serialization_tests.cpp
        )
        target_link_libraries(stormengine_scene_serialization_tests PRIVATE Storm::Serialization)
        target_compile_features(stormengine_scene_serialization_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_scene_serialization_tests)
        add_test(NAME stormengine_scene_serialization_tests COMMAND stormengine_scene_serialization_tests)

        add_executable(stormengine_one_d_physics_tests
            tests/one_d_physics_tests.cpp
        )
        target_link_libraries(stormengine_one_d_physics_tests PRIVATE Storm::Physics1D)
        target_compile_features(stormengine_one_d_physics_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_one_d_physics_tests)
        add_test(NAME stormengine_one_d_physics_tests COMMAND stormengine_one_d_physics_tests)

        add_executable(stormengine_two_d_physics_tests
            tests/two_d_physics_tests.cpp
        )
        target_link_libraries(stormengine_two_d_physics_tests PRIVATE Storm::Physics2D)
        target_compile_features(stormengine_two_d_physics_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_two_d_physics_tests)
        add_test(NAME stormengine_two_d_physics_tests COMMAND stormengine_two_d_physics_tests)

        add_executable(stormengine_one_d_scene_tests
            tests/one_d_scene_tests.cpp
        )
        target_link_libraries(stormengine_one_d_scene_tests PRIVATE Storm::PhysicsScene1D)
        target_compile_features(stormengine_one_d_scene_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_one_d_scene_tests)
        add_test(NAME stormengine_one_d_scene_tests COMMAND stormengine_one_d_scene_tests)

        add_executable(stormengine_two_d_scene_tests
            tests/two_d_scene_tests.cpp
        )
        target_link_libraries(stormengine_two_d_scene_tests PRIVATE Storm::PhysicsScene2D)
        target_compile_features(stormengine_two_d_scene_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_two_d_scene_tests)
        add_test(NAME stormengine_two_d_scene_tests COMMAND stormengine_two_d_scene_tests)

        add_executable(stormengine_transform2d_tests tests/transform2d_tests.cpp)
        target_link_libraries(stormengine_transform2d_tests PRIVATE Storm::Core)
        target_compile_features(stormengine_transform2d_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_transform2d_tests)
        add_test(NAME stormengine_transform2d_tests COMMAND stormengine_transform2d_tests)

        add_executable(stormengine_transform2d_serialization_tests
            tests/transform2d_serialization_tests.cpp)
        target_link_libraries(stormengine_transform2d_serialization_tests PRIVATE Storm::Serialization)
        target_compile_features(stormengine_transform2d_serialization_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_transform2d_serialization_tests)
        add_test(NAME stormengine_transform2d_serialization_tests
            COMMAND stormengine_transform2d_serialization_tests)

        add_executable(stormengine_touch_input_tests
            tests/touch_input_tests.cpp
        )
        target_link_libraries(stormengine_touch_input_tests PRIVATE Storm::Input)
        target_compile_features(stormengine_touch_input_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_touch_input_tests)
        add_test(NAME stormengine_touch_input_tests COMMAND stormengine_touch_input_tests)

        add_executable(stormengine_gesture_recognizer_tests
            tests/gesture_recognizer_tests.cpp
        )
        target_link_libraries(stormengine_gesture_recognizer_tests PRIVATE Storm::Input)
        target_compile_features(stormengine_gesture_recognizer_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_gesture_recognizer_tests)
        add_test(NAME stormengine_gesture_recognizer_tests
            COMMAND stormengine_gesture_recognizer_tests)


        add_executable(stormengine_input_action_map_tests
            tests/input_action_map_tests.cpp
        )
        target_link_libraries(stormengine_input_action_map_tests PRIVATE Storm::Input)
        target_compile_features(stormengine_input_action_map_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_input_action_map_tests)
        add_test(NAME stormengine_input_action_map_tests
            COMMAND stormengine_input_action_map_tests)

        add_executable(stormengine_touch_input_game_loop_adapter_tests
            tests/touch_input_game_loop_adapter_tests.cpp
        )
        target_link_libraries(stormengine_touch_input_game_loop_adapter_tests PRIVATE Storm::Runtime)
        target_compile_features(stormengine_touch_input_game_loop_adapter_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_touch_input_game_loop_adapter_tests)
        add_test(NAME stormengine_touch_input_game_loop_adapter_tests
            COMMAND stormengine_touch_input_game_loop_adapter_tests)

        add_executable(stormengine_gesture_action_input_pipeline_tests
            tests/gesture_action_input_pipeline_tests.cpp
        )
        target_link_libraries(stormengine_gesture_action_input_pipeline_tests PRIVATE Storm::Runtime)
        target_compile_features(stormengine_gesture_action_input_pipeline_tests PRIVATE cxx_std_20)
        stormengine_apply_warnings(stormengine_gesture_action_input_pipeline_tests)
        add_test(NAME stormengine_gesture_action_input_pipeline_tests
            COMMAND stormengine_gesture_action_input_pipeline_tests)
    endif()
endif()
