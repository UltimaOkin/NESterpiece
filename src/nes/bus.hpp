#pragma once
#include <cinttypes>
#include <array>
#include <memory>

namespace NESterpiece
{
	class Cartridge;
	class PPU;
	enum BusActivityType
	{
		Read = 0,
		Write = 1
	};
	struct BusActivity
	{
		uint16_t address = 0;
		uint16_t value = 0;
		BusActivityType type = BusActivityType::Read;
	};

	class Bus
	{
		PPU &ppu;

	public:
		Bus(PPU &ppu) : ppu(ppu) {}
		std::array<uint8_t, 0x800> internal_ram{};
		std::shared_ptr<Cartridge> cart{};
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t value);
	};
}