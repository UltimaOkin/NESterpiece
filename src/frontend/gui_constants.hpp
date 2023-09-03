#pragma once
#include <imgui.h>
#include <string>
#include <filesystem>
#include <array>
namespace NESterpiece
{
	std::string get_current_directory();
	std::string get_full_path(std::string file_location);
	const std::string OPEN_SANS_SEMIBOLD_PATH = get_full_path("/fonts/Open_Sans/OpenSans-SemiBold.ttf");
	constexpr ImVec4 MENU_TEXT_DARK = {0.45f, 0.45f, 0.45f, 1};
	constexpr float FONT_SIZE = 19.0f;
	constexpr float FONT_RENDER_SIZE = 22.0f;

	constexpr std::chrono::nanoseconds set_update_frequency_hz(size_t hz)
	{
		using namespace std::chrono_literals;
		constexpr std::chrono::nanoseconds nanoseconds_per_hertz = 1000000000ns;
		// framerate was inconsistent without this adjustment
		return nanoseconds_per_hertz / hz + 1ns;
	}
}