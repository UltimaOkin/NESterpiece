#pragma once
#include <cinttypes>
#include <array>
#include <bitset>
#include <string>
namespace NESterpiece
{
	enum Flags6
	{
		MirrorMode = 1,
		BatteryRAM = 2,
		HasTrainer = 4,
		IgnoreMirror = 8,
	};

	struct INESHeader
	{
		std::array<char, 4> magic;
		uint8_t prog_rom_size = 0;
		uint8_t chr_rom_size = 0; // 0 = chr ram
		std::bitset<8> flags_6;
	};

	class NROM
	{
	public:
		std::array<uint8_t, 32768> ram;
		std::array<uint8_t, 32768> rom;
		std::array<uint8_t, 32768> chr_rom;
		void from_file(std::string path);
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t value);
	};
}