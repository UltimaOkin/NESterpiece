#pragma once
#include "cpu.hpp"
#include "bus.hpp"
#include "ppu.hpp"
#include <cinttypes>
namespace NESterpiece
{
	class Core
	{
		uint8_t cpu_counter = 0, ppu_counter = 0;

	public:
		CPU cpu;
		PPU ppu;
		Bus bus;
		Core();
		void tick_components(uint32_t rate);
		void tick_until_vblank();
	};
}