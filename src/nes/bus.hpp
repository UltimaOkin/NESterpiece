#pragma once
#include "pad.hpp"
#include <cinttypes>
#include <array>
#include <memory>

namespace NESterpiece
{
	class Cartridge;
	class PPU;
	class OAMDMA;
	class Core;

	enum BusActivityType
	{
		Read = 0,
		Write = 1
	};
	struct BusActivity
	{
		uint8_t value = 0;
		uint16_t address = 0;
		BusActivityType type = BusActivityType::Read;
	};

	class Bus
	{
		PPU &ppu;
		OAMDMA &oam_dma;
		Core &core;

	public:
		BusActivity activity;
		StdController pad;
		Bus(PPU &ppu, OAMDMA &oam_dma, Core &core) : ppu(ppu), oam_dma(oam_dma), core(core) {}
		std::array<uint8_t, 0x800> internal_ram{};
		std::shared_ptr<Cartridge> cart{};
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t value);

		uint8_t read_no_tick(uint16_t address);
		void write_no_tick(uint16_t address, uint8_t value);
	};

}