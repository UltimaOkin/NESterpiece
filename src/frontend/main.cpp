#include <fmt/format.h>
#include <nes/core.hpp>
#include <nes/cartridge.hpp>
#include <nes/ppu.hpp>
#include <SDL.h>
#include <array>
#include <chrono>

constexpr std::chrono::nanoseconds set_update_frequency_hz(size_t hz)
{
	using namespace std::chrono_literals;
	constexpr std::chrono::nanoseconds nanoseconds_per_hertz = 1000000000ns;
	// framerate was inconsistent without this adjustment
	return nanoseconds_per_hertz / hz + 1ns;
}

int main(int argc, char **argv)
{
	using namespace NESterpiece;
	using namespace std::chrono_literals;
	if (argc < 2)
	{
		fmt::print("No rom provided.");
		return 0;
	}

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		fmt::print("Unable to initialize SDL2 Components.\n");
		return 0;
	}

#ifdef WIN32
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
#endif

	SDL_Window *window = SDL_CreateWindow("NESterpiece", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 480, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

	auto cart = Cartridge::from_file(argv[1]);
	Core core{};
	core.bus.cart = cart;
	core.cpu.reset();

	bool running = true;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	constexpr std::chrono::nanoseconds max_step_delay = 33ms;
	constexpr auto logic_rate = set_update_frequency_hz(60);
	auto accumulator = 0ns, sram_accumulator = 0ns;
	auto old_time = std::chrono::steady_clock::now();

	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
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

		if (accumulator >= logic_rate)
		{
			core.tick_until_vblank();

			accumulator -= logic_rate;
		}

		SDL_Rect src{};
		src.y = 0;
		src.w = 256 * 2;
		src.h = 240 * 2;

		SDL_RenderClear(renderer);
		SDL_UpdateTexture(texture, nullptr, core.ppu.framebuffer.data(), 256 * sizeof(uint32_t));
		SDL_RenderCopy(renderer, texture, nullptr, &src);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}