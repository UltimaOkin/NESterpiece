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
	core.cpp
	cpu.cpp
	bus.cpp
	cartridge.cpp
	ppu.cpp
	oam.cpp
	pad.cpp
)