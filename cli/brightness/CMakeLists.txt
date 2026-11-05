cmake_minimum_required(VERSION 3.10)
project(brightness LANGUAGES CXX)

find_package(PkgConfig REQUIRED)

pkg_check_modules(libsystemd REQUIRED IMPORTED_TARGET libsystemd)

add_executable(brightness ./main.cc ./brightness.cc)

target_link_libraries(brightness PRIVATE PkgConfig::libsystemd)

install(TARGETS brightness DESTINATION bin)