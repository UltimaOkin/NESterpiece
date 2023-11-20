#pragma once
#include "cpu.hpp"
#include "bus.hpp"
#include "ppu.hpp"
#include <cinttypes>
#include <memory>
namespace NESterpiece
{
	class Cartridge;
	class Core
	{
		uint8_t cpu_counter = 0, ppu_counter = 0;

	public:
		CPU cpu;
		PPU ppu;
		Bus bus;
		Core();
		void reset(std::shared_ptr<Cartridge> cart);
		void tick_components(bool read_cycle);
		void tick_until_vblank();
	};
}