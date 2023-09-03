#include "menu_bar.hpp"
#include "menu.hpp"
#include "../config.hpp"
#include "../state.hpp"
#include "../gui_constants.hpp"
#include <imgui.h>
#include <nfd.hpp>
#include <SDL.h>
namespace NESterpiece
{
	void MenuBar::draw(EmulationState &state, MenuController &menu_controller)
	{
		height = 0;
		if (!shown)
			return;

		if (ImGui::BeginMainMenuBar())
		{
			height = ImGui::GetWindowSize().y;
			system_menu(state, menu_controller);
			emulation_menu(state);
			view_menu(state);

			ImGui::EndMainMenuBar();
		}
		state.menu_bar_height = static_cast<int32_t>(height);
	}
	void MenuBar::system_menu(EmulationState &state, MenuController &menu_controller)
	{
		using namespace ImGui;
		auto &config = Configuration::get();
		std::string successful_rom_loaded;

		if (BeginMenu("System"))
		{
			TextColored(MENU_TEXT_DARK, "Cartridge");
			Spacing();
			if (MenuItem("Load"))
			{
				NFD::UniquePath out_path;

				if (NFD::OpenDialog(out_path) == nfdresult_t::NFD_OKAY)
				{
					if (state.try_play(out_path.get()))
						successful_rom_loaded = out_path.get();
				}
			}

			if (BeginMenu("Load Recent", !config.recent_rom_paths.empty()))
			{
				for (const auto &path : config.recent_rom_paths)
				{
					if (MenuItem(path.c_str()))
					{
						if (state.try_play(path))
							successful_rom_loaded = path;
					}
				}
				EndMenu();
			}
			Separator();
			TextColored(MENU_TEXT_DARK, "Settings");
			Spacing();
			if (MenuItem("Emulation"))
				menu_controller.toggle_settings(MenuSelect::Emulation);
			if (MenuItem("Input"))
				menu_controller.toggle_settings(MenuSelect::Input);

			Separator();
			Spacing();
			if (MenuItem("Exit"))
				ready_to_exit = true;
			EndMenu();
		}

		if (!successful_rom_loaded.empty())
			config.add_rom_path(std::move(successful_rom_loaded));
	}
	void MenuBar::emulation_menu(EmulationState &state)
	{
		using namespace ImGui;
		auto &config = Configuration::get();
		if (BeginMenu("Emulation"))
		{
			TextColored(MENU_TEXT_DARK, "CPU State");
			Spacing();
			if (MenuItem("Reset"))
				state.reset();

			if (MenuItem("Toggle Pause", nullptr, state.paused))
				state.toggle_pause();

			if (MenuItem("Stop"))
			{
				state.status = Status::Stopped;
				state.cart.reset();
			}

			EndMenu();
		}
	}
	void MenuBar::view_menu(EmulationState &state)
	{
		using namespace ImGui;
		auto &config = Configuration::get();
		if (BeginMenu("View"))
		{
			TextColored(MENU_TEXT_DARK, "Screen");
			Spacing();
			if (BeginMenu("Scale"))
			{
				if (MenuItem("1x"))
					SDL_SetWindowSize(state.window(), SCREEN_WIDTH, SCREEN_HEIGHT + static_cast<int32_t>(height));
				if (MenuItem("2x"))
					SDL_SetWindowSize(state.window(), SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2 + static_cast<int32_t>(height));
				if (MenuItem("3x"))
					SDL_SetWindowSize(state.window(), SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3 + static_cast<int32_t>(height));
				if (MenuItem("4x"))
					SDL_SetWindowSize(state.window(), SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4 + static_cast<int32_t>(height));
				if (MenuItem("5x"))
					SDL_SetWindowSize(state.window(), SCREEN_WIDTH * 5, SCREEN_HEIGHT * 5 + static_cast<int32_t>(height));
				EndMenu();
			}

			if (MenuItem("Linear Filtering", nullptr, config.video.linear_filtering))
				state.change_filter_mode(!config.video.linear_filtering);
			EndMenu();
		}
	}

}