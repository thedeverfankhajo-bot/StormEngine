cmake_minimum_required(VERSION 3.25)

project(
    StormEngine
    VERSION 0.1.0
    DESCRIPTION "An independent, modular, cross-platform game engine"
    LANGUAGES CXX
)

include(GNUInstallDirs)

option(STORM_ENABLE_WARNINGS "Enable strict compiler warnings for StormEngine targets" ON)
option(STORM_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CROSSCOMPILING)
    set(_storm_tests_default ON)
else()
    set(_storm_tests_default OFF)
endif()
option(STORM_BUILD_TESTS "Build StormEngine tests" ${_storm_tests_default})

add_library(storm_core STATIC
    src/core/version.cpp
)
add_library(Storm::Core ALIAS storm_core)

set_target_properties(storm_core PROPERTIES
    EXPORT_NAME Core
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(storm_core PUBLIC cxx_std_20)
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
        target_compile_options(storm_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(storm_core INTERFACE -fsanitize=address,undefined)
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

    add_test(NAME storm_core_tests COMMAND storm_core_tests)
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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StormEngine
)
