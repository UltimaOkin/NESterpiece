
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(CPUTests main.cpp)
set_target_properties(CPUTests PROPERTIES
	CXX_STANDARD 20
	RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}/bin_tests>"
)

if(MSVC_USE_STATIC_CRT)
	set_target_properties(CPUTests PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	)
else()
	set_target_properties(CPUTests PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
	)
endif()

target_include_directories(CPUTests PRIVATE ../src)
target_link_libraries(CPUTests PRIVATE NESterpiece-Core)
target_link_libraries(CPUTests PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(CPUTests PRIVATE fmt::fmt)

message(STATUS "DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME CPUTests COMMAND CPUTests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")