find_package(imgui CONFIG)
find_package(fmt CONFIG)
find_package(SDL2 CONFIG)
find_package(SDL2_mixer CONFIG)

if(imgui_FOUND AND fmt_FOUND AND SDL2_FOUND AND SDL2_mixer_FOUND)
	message(STATUS "Frontend for NESterpiece is enabled.")
	add_executable(NESterpiece main.cpp)
	target_sources(NESterpiece PRIVATE
		config.cpp
		gui_constants.cpp
		state.cpp
		input.cpp
		menu/menu.cpp
		menu/menu_bar.cpp
		menu/settings_menu.cpp
		menu/input_menu.cpp
		menu/emulation_menu.cpp
		menu/diagnostics.cpp
	)
	set_target_properties(NESterpiece PROPERTIES
		CXX_STANDARD 20
		RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_SOURCE_DIR}/bin>"
		OUTPUT_NAME "NESterpiece"
	)

	if(MSVC_USE_STATIC_CRT)
		set_target_properties(NESterpiece PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		)

		set_target_properties(nfd PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		)
		set_target_properties(tomlplusplus_tomlplusplus PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		)
	else()
		set_target_properties(NESterpiece PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
		)
		set_target_properties(nfd PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
		)
		set_target_properties(tomlplusplus_tomlplusplus PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
		)
	endif()

	target_include_directories(NESterpiece PRIVATE ../ ../../extern/tomlplusplus)
	target_link_libraries(NESterpiece PRIVATE NESterpiece-Core SDL2::SDL2 SDL2::SDL2main imgui::imgui fmt::fmt nfd tomlplusplus::tomlplusplus)
	target_link_libraries(NESterpiece PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)

	configure_file(../../fonts/Open_Sans/OpenSans-SemiBold.ttf ${CMAKE_SOURCE_DIR}/bin/fonts/Open_Sans/OpenSans-SemiBold.ttf COPYONLY)
	configure_file(../../fonts/Open_Sans/OFL.txt ${CMAKE_SOURCE_DIR}/bin/fonts/Open_Sans/OFL.txt COPYONLY)
endif()