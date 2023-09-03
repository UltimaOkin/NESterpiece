#include "state.hpp"
#include "config.hpp"
#include <nes/constants.hpp>
#include <nes/cartridge.hpp>
#include <SDL.h>
#include <cmath>
namespace NESterpiece
{
	EmulationState::EmulationState(SDL_Window *window)
		: _window(window)
	{
	}

	EmulationState::~EmulationState()
	{
		_window = nullptr;
		close();
	}

	void EmulationState::initialize(SDL_Renderer *renderer)
	{
		create_texture(renderer);
	}

	void EmulationState::close()
	{
		if (texture)
		{
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
	}

	SDL_Window *EmulationState::window() const
	{
		return _window;
	}

	void EmulationState::create_texture(SDL_Renderer *renderer)
	{
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
		auto &config = Configuration::get();

		if (config.video.linear_filtering)
			SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
		else
			SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
	}

	void EmulationState::change_filter_mode(bool use_linear_filter)
	{
		auto &config = Configuration::get();
		config.video.linear_filtering = use_linear_filter;

		if (config.video.linear_filtering)
			SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
		else
			SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
	}

	bool EmulationState::try_play(std::string_view path)
	{
		cart = Cartridge::from_file(path.data());
		auto &config = Configuration::get();
		if (cart)
		{
			paused = false;
			core.reset(cart);
			status = Status::Running;
			return true;
		}
		return false;
	}

	void EmulationState::reset()
	{
		auto &config = Configuration::get();
		if (cart)
		{
			paused = false;
			core.reset(cart);
			status = Status::Running;
		}
	}

	void EmulationState::toggle_pause()
	{
		if (status == Status::Running)
			paused = !paused;
	}

	void EmulationState::poll_input()
	{
		core.bus.pad.reset();
		user_input.update_state(core.bus.pad);
	}

	void EmulationState::step_frame()
	{
		if (status == Status::Running && !paused)
			core.tick_until_vblank();
	}

	void EmulationState::draw_frame(SDL_Window *window, SDL_Renderer *renderer)
	{
		if (status == Status::Running && texture)
		{
			const auto &framebuffer_pixels = core.ppu.framebuffer;
			SDL_UpdateTexture(texture, nullptr, framebuffer_pixels.data(), SCREEN_WIDTH * sizeof(uint32_t));

			int32_t w = 0, h = 0;
			SDL_GetWindowSize(window, &w, &h);

			h = h - menu_bar_height;
			SDL_Rect src{0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
			if (Configuration::get().video.keep_aspect_ratio)
			{
				auto final_width = w, final_height = h;
				auto scaled_w = static_cast<int32_t>(static_cast<float>(w) * (1.0f / 1.07f));
				auto scaled_h = static_cast<int32_t>(static_cast<float>(h) * (1.07f / 1.0f));

				if (scaled_h <= w)
				{
					final_width = scaled_h;
				}
				else if (scaled_w <= h)
				{
					final_height = scaled_w;
				}

				SDL_Rect rect{(w / 2) - (final_width / 2), menu_bar_height, final_width, final_height};
				SDL_RenderCopy(renderer, texture, &src, &rect);
			}
			else
			{
				SDL_Rect rect{0, menu_bar_height, w, h};
				SDL_RenderCopy(renderer, texture, &src, &rect);
			}
		}
	}
}