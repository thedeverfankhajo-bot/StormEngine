cmake_minimum_required(VERSION 3.25)

project(StormEnginePackageConsumer LANGUAGES CXX)

find_package(StormEngine 0.1 CONFIG REQUIRED)

add_executable(storm_package_consumer main.cpp)
target_link_libraries(storm_package_consumer PRIVATE Storm::Core)
target_compile_features(storm_package_consumer PRIVATE cxx_std_20)
set_target_properties(storm_package_consumer PROPERTIES CXX_EXTENSIONS OFF)
