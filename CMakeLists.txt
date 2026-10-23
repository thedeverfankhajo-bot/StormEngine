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
                "-DSTORM_TEST_CONFIG=$<CONFIG>"
                "-DSTORM_GENERATOR=${CMAKE_GENERATOR}"
                "-DSTORM_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                "-DSTORM_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
                -P "${PROJECT_SOURCE_DIR}/tests/cmake/package_consumer_test.cmake"
        )
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
