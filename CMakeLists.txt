cmake_minimum_required(VERSION 3.25)

project(
    StormEngine
    VERSION 0.1.0
    DESCRIPTION "An independent, modular, cross-platform game engine"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(STORM_ENABLE_WARNINGS "Enable strict compiler warnings for StormEngine targets" ON)
option(STORM_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(STORM_BUILD_SDL3_DEMO "Build the optional SDL3 desktop 2D bootstrap demo" OFF)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CROSSCOMPILING)
    set(_storm_tests_default ON)
else()
    set(_storm_tests_default OFF)
endif()
option(STORM_BUILD_TESTS "Build StormEngine tests" ${_storm_tests_default})

# Provide a relocatable CMake package for downstream projects.
set(STORM_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/StormEngine")
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/StormEngineConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/StormEngineConfig.cmake"
    INSTALL_DESTINATION "${STORM_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/StormEngineConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)

add_library(storm_core STATIC
    src/core/version.cpp
    src/core/stable_id_generator.cpp
    src/scene/scene_file.cpp
    src/render/software_canvas2d.cpp
    src/render/texture2d.cpp
    src/render/camera2d.cpp
    src/render/scene_renderer2d.cpp
)
add_library(Storm::Core ALIAS storm_core)

set_target_properties(storm_core PROPERTIES
    EXPORT_NAME Core
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(storm_core PUBLIC cxx_std_20)

# The generator uses native platform cryptography APIs; no third-party runtime dependency.
if(WIN32)
    target_link_libraries(storm_core PRIVATE bcrypt)
elseif(APPLE)
    find_library(STORM_SECURITY_FRAMEWORK Security REQUIRED)
    target_link_libraries(storm_core PRIVATE "${STORM_SECURITY_FRAMEWORK}")
endif()
target_include_directories(storm_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(STORM_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(storm_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(storm_core PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
        )
    endif()
endif()

if(STORM_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        set(_storm_sanitizer_compile_options -fsanitize=address,undefined -fno-omit-frame-pointer)
        set(_storm_sanitizer_link_options -fsanitize=address,undefined)
        target_compile_options(storm_core PRIVATE ${_storm_sanitizer_compile_options})
        target_link_options(storm_core INTERFACE ${_storm_sanitizer_link_options})
    else()
        message(FATAL_ERROR
            "STORM_ENABLE_SANITIZERS currently supports GCC and Clang-family compilers only."
        )
    endif()
endif()

if(STORM_BUILD_TESTS)
    enable_testing()

    add_executable(storm_core_tests
        tests/test_main.cpp
    )
    target_link_libraries(storm_core_tests PRIVATE Storm::Core)
    target_compile_features(storm_core_tests PRIVATE cxx_std_20)
    set_target_properties(storm_core_tests PROPERTIES CXX_EXTENSIONS OFF)

    if(STORM_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(storm_core_tests PRIVATE /W4 /permissive-)
        else()
            target_compile_options(storm_core_tests PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
            )
        endif()
    endif()

    # The tests exercise many header-only primitives, so instrument their translation unit too.
    if(STORM_ENABLE_SANITIZERS)
        target_compile_options(storm_core_tests PRIVATE ${_storm_sanitizer_compile_options})
    endif()

    add_test(NAME storm_core_tests COMMAND storm_core_tests)

    if(NOT CMAKE_CROSSCOMPILING)
        # Exercise the installed package from a separate, relocated consumer project.
        add_test(
            NAME storm_package_consumer
            COMMAND "${CMAKE_COMMAND}"
                "-DSTORM_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
                "-DSTORM_BUILD_DIR=${PROJECT_BINARY_DIR}"
                "-DSTORM_BINARY_DIR=${PROJECT_BINARY_DIR}"
                "-DSTORM_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}"
                "-DSTORM_TEST_CONFIG=$<CONFIG>"
                "-DSTORM_GENERATOR=${CMAKE_GENERATOR}"
                "-DSTORM_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                "-DSTORM_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
                -P "${PROJECT_SOURCE_DIR}/tests/cmake/package_consumer_test.cmake"
        )
    endif()
endif()


if(STORM_BUILD_SDL3_DEMO)
    if(ANDROID)
        message(FATAL_ERROR
            "STORM_BUILD_SDL3_DEMO is a desktop executable, not an Android APK target. "
            "Android SDL lifecycle and packaging need a dedicated application entry point."
        )
    endif()

    include(FetchContent)

    # The normal core build stays dependency-free. Opting into the sample fetches a checksum-pinned
    # upstream release and builds SDL statically without its own tests, examples or install rules.
    set(SDL_SHARED OFF CACHE BOOL "Build SDL3 shared library" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build SDL3 static library" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL3 test library" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL3 examples" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "Install SDL3 artifacts" FORCE)
    set(SDL_INSTALL_DOCS OFF CACHE BOOL "Install SDL3 documentation" FORCE)
    set(SDL_INSTALL_TESTS OFF CACHE BOOL "Install SDL3 tests" FORCE)

    FetchContent_Declare(
        storm_sdl3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
        URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(storm_sdl3)

    add_executable(storm_sdl3_demo examples/sdl3_2d_demo/main.cpp)
    target_link_libraries(storm_sdl3_demo PRIVATE Storm::Core SDL3::SDL3)
    target_compile_features(storm_sdl3_demo PRIVATE cxx_std_20)
    set_target_properties(storm_sdl3_demo PROPERTIES CXX_EXTENSIONS OFF)

    if(STORM_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(storm_sdl3_demo PRIVATE /W4 /permissive-)
        else()
            target_compile_options(storm_sdl3_demo PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
            )
        endif()
    endif()
endif()

install(
    TARGETS storm_core
    EXPORT StormEngineTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
    EXPORT StormEngineTargets
    FILE StormEngineTargets.cmake
    NAMESPACE Storm::
    DESTINATION "${STORM_INSTALL_CMAKEDIR}"
)
install(
    FILES
        "${PROJECT_BINARY_DIR}/StormEngineConfig.cmake"
        "${PROJECT_BINARY_DIR}/StormEngineConfigVersion.cmake"
    DESTINATION "${STORM_INSTALL_CMAKEDIR}"
)


# Redistribute the engine license and applicable third-party notices with installed SDKs.
install(
    FILES
        "${PROJECT_SOURCE_DIR}/LICENSE"
        "${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/StormEngine"
)
