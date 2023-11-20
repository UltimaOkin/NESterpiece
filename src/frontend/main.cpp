#include "menu/menu.hpp"
#include "menu/style.hpp"
#include "gui_constants.hpp"
#include "state.hpp"
#include "config.hpp"
#include <nes/constants.hpp>
#include <string>
#include <fmt/format.h>
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <nfd.hpp>
#include <chrono>
#include <iostream>

int main(int argc, char **argv)
{
	using namespace std::chrono_literals;
	using namespace NESterpiece;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0)
	{
		fmt::print("Unable to initialize SDL2 Components\n");
		return 0;
	}

	NFD::Init();

#ifdef WIN32
	// d3d12 crashes when resizing and the default is d3d9
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
#endif

	SDL_Window *window = SDL_CreateWindow("NESterpiece", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * 3, (SCREEN_HEIGHT * 3) + 25, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT + 25);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	ImGui::CreateContext();
	SetupImGuiStyle();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.FontDefault = io.Fonts->AddFontFromFileTTF(OPEN_SANS_SEMIBOLD_PATH.c_str(), FONT_RENDER_SIZE);
	io.FontGlobalScale = FONT_SIZE / FONT_RENDER_SIZE;

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	bool running = true;
	constexpr std::chrono::nanoseconds max_step_delay = 33ms;
	constexpr auto logic_rate = set_update_frequency_hz(60);
	auto accumulator = 0ns;
	auto old_time = std::chrono::steady_clock::now();

	MenuController menu;
	auto &config = Configuration::get();
	EmulationState state{window};

	state.initialize(renderer);
	state.core.ppu.update_event = SnapshotEvent::OnScanlineCycle;
	state.core.ppu.trigger_cycle = 0;
	state.core.ppu.trigger_scanline = 0;
	ControllerHandler::open();
	while (running && !menu.menu_bar.ready_to_exit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type)
			{
			case SDL_EventType::SDL_CONTROLLERDEVICEADDED:
			case SDL_EventType::SDL_CONTROLLERDEVICEREMOVED:
			{
				ControllerHandler::close();
				ControllerHandler::open();
				break;
			}

			case SDL_QUIT:
			{
				running = false;
				break;
			}
			case SDL_WINDOWEVENT:
			{
				if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
					event.window.windowID == SDL_GetWindowID(window))
				{
					running = false;
				}

				break;
			}
			}
		}

		auto time_now = std::chrono::steady_clock::now();
		auto full_delta = time_now - old_time;
		old_time = time_now;

		if (full_delta > max_step_delay)
			full_delta = max_step_delay;

		accumulator += full_delta;

		state.poll_input();

		if (accumulator >= logic_rate)
		{
			if (state.status == NESterpiece::Status::Running)
				state.step_frame();

			accumulator -= logic_rate;
		}

		SDL_RenderClear(renderer);

		if (state.status == NESterpiece::Status::Running)
			state.draw_frame(window, renderer);

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		menu.draw(state);
		ImGui::Render();
		SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);
	}
	ControllerHandler::close();
	config.save_as_toml_file();

	state.close();
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	NFD::Quit();
	SDL_Quit();

	return 0;
}