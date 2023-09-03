cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT})
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

configure_file(LICENSE ${CMAKE_SOURCE_DIR}/bin/LICENSE COPYONLY)
option(MSVC_USE_STATIC_CRT "Enable MultiThreaded Runtime Library for MSVC" ON)
project(NESterpiece LANGUAGES CXX C)
enable_testing()
add_subdirectory(extern/nativefiledialog-extended)
add_subdirectory(extern/tomlplusplus)
add_subdirectory(src)
add_subdirectory(cpu_tests)