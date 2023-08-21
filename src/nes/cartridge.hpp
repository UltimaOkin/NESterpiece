#pragma once
#include <cinttypes>
#include <array>
#include <bitset>
#include <string>
#include <memory>
#include <fstream>

namespace NESterpiece
{
	enum class ConsoleType
	{
		NES,
		NESVsSystem,
		Playchoice10,
		ExtendedConsole,
	};

	enum class HardwareTiming
	{
		RP2C02, // NTSC NES
		RP2C07, // PAL NES
		MultipleRegion,
		UA6538, // Dendy
	};

	class Flags6
	{
	public:
		uint8_t data = 0;
		uint8_t mirror() const { return data & 1; }
		uint8_t has_battery() const { return (data & 2) >> 1; }
		uint8_t has_trainer() const { return (data & 4) >> 2; }
		uint8_t is_four_screen() const { return (data & 8) >> 3; }
		uint8_t mapper_first_nibble() const { return (data & 240) >> 4; }
	};

	class Flags7
	{
	public:
		uint8_t data = 0;
		ConsoleType console_type() const { return static_cast<ConsoleType>(data & 3); }
		bool is_ines_2_0() const { return (data & 12) == 8; }
		uint8_t mapper_second_nibble() const { return (data & 240) >> 4; }
	};

	class MapperInfo
	{
	public:
		uint8_t data = 0;
		uint8_t mapper_third_nibble() const { return data & 15; }
		uint8_t submapper_number() const { return data & 240 >> 4; }
	};

	class PrgChrSize
	{
	public:
		uint8_t data = 0;
		uint8_t prg_size_msb() const { return data & 15; }
		uint8_t chr_size_msb() const { return data & 240 >> 4; }
	};

	class PrgRamSize
	{
		// actual size = 64 << shift
		// if shift == 0 then no ram/eeprom is used
	public:
		uint8_t data = 0;
		uint8_t prg_ram_shift() const { return data & 15; }
		uint8_t prg_eeprom_shift() const { return data & 240 >> 4; }
	};

	class ChrRamSize
	{
		// actual size = 64 << shift
		// if shift == 0 then no ram/nvram is used
	public:
		uint8_t data = 0;
		uint8_t chr_ram_shift() const { return data & 15; }
		uint8_t chr_nvram_shift() const { return data & 240 >> 4; }
	};

	struct INESHeader
	{
		uint8_t prg_rom_low_byte = 0;
		uint8_t chr_rom_low_byte = 0; // 0 = chr ram
		Flags6 flags_6;
		Flags7 flags_7;
		MapperInfo m_info;
		PrgChrSize rom_sizes;
		PrgRamSize prg_ram_sizes;
		ChrRamSize chr_ram_sizes;
		HardwareTiming timing;
		uint8_t vs_system_info;
		uint8_t extended_console_info;
		uint8_t num_misc_roms = 0;
		uint8_t default_expansion_device = 0;

		uint16_t combined_mapper_id() const { return flags_6.mapper_first_nibble() | (flags_7.mapper_second_nibble() << 4) | (m_info.mapper_third_nibble() << 8); }
	};

	class Cartridge
	{
	public:
		INESHeader header;
		Cartridge(INESHeader &&header) : header(std::move(header)) {}

		virtual uint8_t read(uint16_t address) = 0;
		virtual void write(uint16_t address, uint8_t value) = 0;
		virtual uint8_t read_chr(uint16_t address) = 0;
		virtual uint8_t read_chr(uint16_t pattern_table_half, uint16_t tile_id, uint16_t bit_plane, uint16_t fine_y) = 0;
		virtual void write_chr(uint16_t address, uint8_t value) = 0;
		virtual uint8_t read_nametable(uint16_t address) = 0;
		virtual void write_nametable(uint16_t address, uint8_t value) = 0;

		static std::shared_ptr<Cartridge> from_file(std::string path);
	};

	class NROM : public Cartridge
	{
	public:
		std::array<uint8_t, 32768> prg_rom{};
		std::array<uint8_t, 8192> prg_ram{};
		std::array<uint8_t, 8192> chr_rom{};
		std::array<uint8_t, 2048> nametables{};

		NROM(INESHeader &&header, std::ifstream stream);

		uint8_t read(uint16_t address) override;
		void write(uint16_t address, uint8_t value) override;
		uint8_t read_chr(uint16_t address) override;
		uint8_t read_chr(uint16_t pattern_table_half, uint16_t tile_id, uint16_t bit_plane, uint16_t fine_y) override;
		void write_chr(uint16_t address, uint8_t value) override;
		uint8_t read_nametable(uint16_t address) override;
		void write_nametable(uint16_t address, uint8_t value) override;
	};
}