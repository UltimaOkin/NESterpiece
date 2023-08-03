cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT})
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

option(MSVC_USE_STATIC_CRT "Enable MultiThreaded Runtime Library for MSVC" ON)
option(BUILD_SUNBOY_FRONTEND "Build Frontend" ON)
project(NESterpiece LANGUAGES CXX C)

add_library(NESterpiece-Core)
set_target_properties(NESterpiece-Core PROPERTIES
	CXX_STANDARD 20
	RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}/bin>"
	OUTPUT_NAME "Nesterpiece-Core"
)

if(MSVC_USE_STATIC_CRT)
	set_target_properties(NESterpiece-Core PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	)
else()
	set_target_properties(NESterpiece-Core PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
	)
endif()

target_sources(NESterpiece-Core PRIVATE
	src/nes/core.cpp
	src/nes/cpu.cpp
)

add_executable(NESterpiece src/frontend/main.cpp)
set_target_properties(NESterpiece PROPERTIES
	CXX_STANDARD 20
	RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}/bin>"
	OUTPUT_NAME "Nesterpiece"
)
target_include_directories(NESterpiece PRIVATE src/)
target_link_libraries(NESterpiece PRIVATE NESterpiece-Core)