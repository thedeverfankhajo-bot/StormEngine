if(NOT DEFINED STORM_SOURCE_DIR OR NOT DEFINED STORM_BUILD_DIR OR
   NOT DEFINED STORM_BINARY_DIR)
    message(FATAL_ERROR "Required StormEngine package-smoke-test paths were not supplied")
endif()

if(NOT DEFINED STORM_TEST_CONFIG OR STORM_TEST_CONFIG STREQUAL "")
    set(STORM_TEST_CONFIG "Debug")
endif()

set(_smoke_root "${STORM_BINARY_DIR}/package-consumer-smoke")
set(_install_prefix "${_smoke_root}/install")
set(_relocated_prefix "${_smoke_root}/relocated")
set(_consumer_build_dir "${_smoke_root}/consumer-build")

# Ensure a stale package or executable cannot make this integration test pass.
file(REMOVE_RECURSE "${_smoke_root}")

set(_install_command
    "${CMAKE_COMMAND}" --install "${STORM_BUILD_DIR}"
    --prefix "${_install_prefix}" --config "${STORM_TEST_CONFIG}"
)
execute_process(
    COMMAND ${_install_command}
    RESULT_VARIABLE _install_result
    OUTPUT_VARIABLE _install_stdout
    ERROR_VARIABLE _install_stderr
)
if(NOT "${_install_result}" STREQUAL "0")
    message(FATAL_ERROR
        "StormEngine install failed (${_install_result}).\n${_install_stdout}\n${_install_stderr}")
endif()

# Relocation catches package configs that accidentally depend on the source/build directory.
file(RENAME "${_install_prefix}" "${_relocated_prefix}" RESULT _rename_result)
if(NOT "${_rename_result}" STREQUAL "0")
    message(FATAL_ERROR "Could not relocate the install tree: ${_rename_result}")
endif()

set(_configure_command
    "${CMAKE_COMMAND}"
    -S "${STORM_SOURCE_DIR}/tests/package_consumer"
    -B "${_consumer_build_dir}"
    "-DCMAKE_PREFIX_PATH=${_relocated_prefix}"
    "-DCMAKE_BUILD_TYPE=${STORM_TEST_CONFIG}"
)
if(DEFINED STORM_GENERATOR AND NOT STORM_GENERATOR STREQUAL "")
    list(APPEND _configure_command -G "${STORM_GENERATOR}")
endif()
if(DEFINED STORM_GENERATOR_PLATFORM AND NOT STORM_GENERATOR_PLATFORM STREQUAL "")
    list(APPEND _configure_command -A "${STORM_GENERATOR_PLATFORM}")
endif()
if(DEFINED STORM_GENERATOR_TOOLSET AND NOT STORM_GENERATOR_TOOLSET STREQUAL "")
    list(APPEND _configure_command -T "${STORM_GENERATOR_TOOLSET}")
endif()

execute_process(
    COMMAND ${_configure_command}
    RESULT_VARIABLE _configure_result
    OUTPUT_VARIABLE _configure_stdout
    ERROR_VARIABLE _configure_stderr
)
if(NOT "${_configure_result}" STREQUAL "0")
    message(FATAL_ERROR
        "StormEngine consumer configure failed (${_configure_result}).\n${_configure_stdout}\n${_configure_stderr}")
endif()

set(_build_command
    "${CMAKE_COMMAND}" --build "${_consumer_build_dir}"
    --config "${STORM_TEST_CONFIG}" --parallel 2
)
execute_process(
    COMMAND ${_build_command}
    RESULT_VARIABLE _build_result
    OUTPUT_VARIABLE _build_stdout
    ERROR_VARIABLE _build_stderr
)
if(NOT "${_build_result}" STREQUAL "0")
    message(FATAL_ERROR
        "StormEngine consumer build failed (${_build_result}).\n${_build_stdout}\n${_build_stderr}")
endif()

if(WIN32)
    set(_executable_suffix ".exe")
else()
    set(_executable_suffix "")
endif()
if(STORM_GENERATOR MATCHES "Visual Studio|Xcode|Multi-Config")
    set(_consumer_executable
        "${_consumer_build_dir}/${STORM_TEST_CONFIG}/storm_package_consumer${_executable_suffix}")
else()
    set(_consumer_executable
        "${_consumer_build_dir}/storm_package_consumer${_executable_suffix}")
endif()
if(NOT EXISTS "${_consumer_executable}")
    message(FATAL_ERROR "Consumer executable was not created: ${_consumer_executable}")
endif()

execute_process(
    COMMAND "${_consumer_executable}"
    RESULT_VARIABLE _run_result
    OUTPUT_VARIABLE _run_stdout
    ERROR_VARIABLE _run_stderr
)
if(NOT "${_run_result}" STREQUAL "0")
    message(FATAL_ERROR
        "StormEngine consumer executable failed (${_run_result}).\n${_run_stdout}\n${_run_stderr}")
endif()

message(STATUS "StormEngine installed-package consumer passed after relocation")
