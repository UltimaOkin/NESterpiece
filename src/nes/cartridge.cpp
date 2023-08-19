#include "cartridge.hpp"
#include "constants.hpp"
#include <fstream>
#include <iostream>
#include <vector>
namespace NESterpiece
{
	std::shared_ptr<Cartridge> Cartridge::from_file(std::string path)
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

		INESHeader header;
		rom_file.read(reinterpret_cast<char *>(&header.prg_rom_low_byte), 1);
		rom_file.read(reinterpret_cast<char *>(&header.chr_rom_low_byte), 1);
		rom_file.read(reinterpret_cast<char *>(&header.flags_6.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.flags_7.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.m_info.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.rom_sizes.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.prg_ram_sizes.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.chr_ram_sizes.data), 1);
		rom_file.read(reinterpret_cast<char *>(&header.timing), 1);
		rom_file.read(reinterpret_cast<char *>(&header.vs_system_info), 1);
		rom_file.read(reinterpret_cast<char *>(&header.num_misc_roms), 1);
		rom_file.read(reinterpret_cast<char *>(&header.default_expansion_device), 1);

		switch (header.combined_mapper_id())
		{
		case 0:
			return std::make_shared<NROM>(std::move(header), std::move(rom_file));
		}

		return nullptr;
	}

	NROM::NROM(INESHeader &&header, std::ifstream rom_file)
		: Cartridge(std::move(header))
	{
		rom_file.read(reinterpret_cast<char *>(prg_rom.data()), 16384 * header.prg_rom_low_byte);
		rom_file.read(reinterpret_cast<char *>(chr_rom.data()), 8192 * header.chr_rom_low_byte);
		rom_file.close();
	}

	uint8_t NROM::read(uint16_t address)
	{
		if (within_range(address, 0x6000, 0x7FFF))
		{
			return prg_ram[address - 0x6000];
		}
		else if (within_range(address, 0x8000, 0xBFFF))
		{
			return prg_rom[address - 0x8000];
		}
		else if (within_range(address, 0xC000, 0xFFFF))
		{
			uint16_t offset = header.chr_rom_low_byte == 2 ? 0x8000 : 0xC000;
			uint16_t faddr = address - offset;
			auto ret = prg_rom.at(address - offset);
			return ret;
		}

		return 0;
	}

	void NROM::write(uint16_t address, uint8_t value)
	{
		if (within_range(address, 0x6000, 0x7FFF))
		{
			prg_ram[address - 0x6000] = value;
		}
	}

	uint8_t NROM::read_chr(uint16_t address)
	{
		uint16_t addr = address;
		return chr_rom[addr];
	}

	uint8_t NROM::read_chr(uint16_t pattern_table_half, uint16_t tile_id, uint16_t bit_plane, uint16_t fine_y)
	{
		pattern_table_half &= 1;
		tile_id &= 0xFF;
		bit_plane &= 1;
		fine_y &= 7;
		uint16_t address = (pattern_table_half << 12) | (tile_id << 4) | (bit_plane << 3) | fine_y;

		return read_chr(address);
	}
}