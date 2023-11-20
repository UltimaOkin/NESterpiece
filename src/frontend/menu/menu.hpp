#pragma once
#include "settings_menu.hpp"
#include "menu_bar.hpp"
#include "diagnostics.hpp"
namespace NESterpiece
{
	class EmulationState;
	class MenuController
	{
	public:
		MenuBar menu_bar;
		SettingsMenu settings;
		PPUDiagnostics p_diag;
		void toggle_settings(MenuSelect menu);
		void draw(EmulationState &state);
	};

}