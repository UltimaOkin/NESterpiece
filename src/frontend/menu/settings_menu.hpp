#pragma once
#include "emulation_menu.hpp"
#include "input_menu.hpp"
#include <nes/pad.hpp>
#include <cinttypes>
#include <array>
#include <string_view>
#include <vector>
#include <imgui.h>

namespace NESterpiece
{
	enum class MenuSelect
	{
		Emulation,
		Video,
		Audio,
		Input
	};

	class SettingsMenu
	{
	public:
		bool show = false;
		MenuSelect selected_menu = MenuSelect::Emulation;
		EmulationMenu emulation;
		InputMenu input_menu;
		void open(MenuSelect menu);
		void draw_menu(float height);
		bool button2(const char *label, bool selected = false);
	};

}