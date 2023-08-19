find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
add_executable(NESterpiece main.cpp)
set_target_properties(NESterpiece PROPERTIES
	CXX_STANDARD 20
	RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}/bin>"
	OUTPUT_NAME "Nesterpiece"
)

if(MSVC_USE_STATIC_CRT)
	set_target_properties(NESterpiece PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	)
else()
	set_target_properties(NESterpiece PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
	)
endif()

target_include_directories(NESterpiece PRIVATE ../)
target_link_libraries(NESterpiece
	PRIVATE
	fmt::fmt
	NESterpiece-Core
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)