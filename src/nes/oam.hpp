#pragma once
#include <cinttypes>

namespace NESterpiece
{
	class Bus;
	class PPU;
	class OAMDMA
	{
		uint16_t bytes_left = 0;
		uint8_t alignment = 0;
		uint16_t total_cycles = 1;

	public:
		bool put_cycle = false, active = false;
		uint8_t data = 0;
		uint16_t address = 0, address_snap = 0;
		void start(uint8_t page);
		void step(Bus &bus, PPU &ppu);
	};
}