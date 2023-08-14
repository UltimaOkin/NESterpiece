#include "cartridge.hpp"
#include "constants.hpp"
#include <fstream>
#include <iostream>
#include <vector>
namespace NESterpiece
{
	void NROM::from_file(std::string path)
	{
		constexpr std::array<char, 4> TARGET_MAGIC{
			'N',
			'E',
			'S',
			'\x1a',
		};

		std::ifstream rom_file(path, std::ios::binary);

		std::array<char, 4> magic{};
		rom_file.read(magic.data(), magic.size());

		if (magic != TARGET_MAGIC)
			std::cout << "file is not an iNES rom\n";

		uint8_t prg_rom_size = 0, chr_rom_size = 0;
		rom_file.read(reinterpret_cast<char *>(&prg_rom_size), 1);
		rom_file.read(reinterpret_cast<char *>(&chr_rom_size), 1);

		uint8_t flags6 = 0;
		rom_file.read(reinterpret_cast<char *>(&flags6), 1);
		bool vertical_mirror = flags6 & 1;
		bool battery_ram = flags6 & 2;
		bool has_trainer = flags6 & 4;
		bool ignore_mirror_bit = flags6 & 8;
		uint8_t mapper_num = flags6 & 0b11110000;

		uint8_t flags7 = 0;
		rom_file.read(reinterpret_cast<char *>(&flags7), 1);
		bool vs_unisystem = flags7 & 1;
		bool play_choice_10 = flags7 & 2;
		uint8_t ines_version = (flags7 & 0b1100) >> 2;
		mapper_num |= (flags7 & 0b11110000) << 4;

		rom_file.seekg(8, std::ios_base::cur);

		std::vector<uint8_t> rom;
		rom.resize(16384 * prg_rom_size);
		rom_file.read(reinterpret_cast<char *>(rom.data()), rom.size());

		std::vector<uint8_t> chr;
		chr.resize(8192 * chr_rom_size);
		rom_file.read(reinterpret_cast<char *>(chr.data()), chr.size());

		if (prg_rom_size == 2)
		{
			std::copy(rom.begin(), rom.end(), this->rom.begin());
		}
		else
		{
			std::copy(rom.begin(), rom.end(), this->rom.begin());
			std::copy(rom.begin(), rom.end(), this->rom.begin() + 16384);
		}

		std::copy(chr.begin(), chr.end(), chr_rom.begin());
	}

	uint8_t NROM::read(uint16_t address)
	{
		if (within_range(address, 0x8000, 0xFFFF))
		{
			return rom[address - 0x8000];
		}
		return 0;
	}

	void NROM::write(uint16_t address, uint8_t value)
	{
	}
}