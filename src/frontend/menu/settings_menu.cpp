#include "settings_menu.hpp"
#include "../config.hpp"
#include <imgui.h>

namespace NESterpiece
{
	void SettingsMenu::open(MenuSelect menu)
	{
		switch (menu)
		{
		case MenuSelect::Emulation:
			emulation.open();
			break;
		case MenuSelect::Input:
			input_menu.open();
			break;

		default:
			return;
		}
		show = true;
		selected_menu = menu;
	}

	void SettingsMenu::draw_menu(float height)
	{
		using namespace ImGui;
		auto size = GetIO().DisplaySize;

		if (show)
		{
			SetNextWindowBgAlpha(0.95f);
			SetNextWindowPos({0, height});
			SetNextWindowSize({size.x, size.y - height});
			if (ImGui::Begin("Settings", &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration))
			{

				{
					BeginChild("Settings Options", ImVec2(120, 0), false);
					if (button2("Emulation", selected_menu == MenuSelect::Emulation))
						open(MenuSelect::Emulation);

					if (button2("Input", selected_menu == MenuSelect::Input))
						open(MenuSelect::Input);

					if (button2("Close"))
					{
						Configuration::get().save_as_toml_file();
						show = false;
					}
					EndChild();
				}

				SameLine();

				{
					BeginChild("Settings Content", ImVec2(), true, ImGuiWindowFlags_AlwaysAutoResize);
					switch (selected_menu)
					{
					case MenuSelect::Emulation:
						emulation.draw();
						break;
					case MenuSelect::Input:
						input_menu.draw();
						break;
					}
					EndChild();
				}
			}
			End();
		}
	}

	bool SettingsMenu::button2(const char *label, bool selected)
	{
		using namespace ImGui;
		auto color = GetStyleColorVec4(ImGuiCol_ButtonHovered);

		bool pressed = false;
		if (selected)
		{
			PushStyleColor(ImGuiCol_Button, color);
			pressed = Button(label, ImVec2(120, 0));
			PopStyleColor();
		}
		else
		{
			PushStyleColor(ImGuiCol_Button, 0);
			pressed = Button(label, ImVec2(120, 0));
			PopStyleColor();
		}
		return pressed;
	}

}