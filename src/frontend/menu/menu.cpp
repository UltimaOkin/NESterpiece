#include "menu.hpp"
#include "../config.hpp"
namespace NESterpiece
{
	void MenuController::toggle_settings(MenuSelect menu)
	{
		if (settings.show && settings.selected_menu == menu)
		{
			Configuration::get().save_as_toml_file();
			settings.show = false;
			return;
		}
		settings.open(menu);
	}

	void MenuController::draw(EmulationState &state)
	{
		menu_bar.draw(state, *this);
		settings.draw_menu(menu_bar.height);

		using namespace ImGui;

		if (Begin("PPU", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			Text("V: 0x0000; T: 0x0000;");
		}
		End();
	}
}