#pragma once
#include <cinttypes>
#include <array>
#include <vector>
#include <tuple>

namespace NESterpiece
{
	class Core;
	enum CtrlFlags
	{
		NametableSelectCtrl = 3,
		VramIncrement = 4,
		OAMPatternAddress = 8,
		BGPatternAddress = 16,
		OAMSize = 32,
		EnableNMI = 128,
	};

	enum MaskFlags
	{
		Grayscale = 1,
		ShowBGOnLeft = 2,
		ShowSpritesOnLeft = 4,
		ShowBG = 8,
		ShowSprites = 16,
		ColorEmphasisBits = 224,
	};

	enum PPUStatusFlags
	{
		SpriteOverflow = 32,
		Sprite0Hit = 64,
		VBlank = 128,
	};

	enum VramMask
	{
		CoarseX = 0b11111,
		CoarseY = 0b11111 << 5,
		NametableSelect = 0b11 << 10,
		NametableX = 0b1 << 10,
		NametableY = 0b1 << 11,
		FineY = 0b111 << 12,
	};

	enum ObjectAttribute
	{
		Palette = 3,
		Priority = 32,
		FlipX = 64,
		FlipY = 128,
	};

	struct FetcherState
	{
		uint8_t nametable_tile = 0, tile_attribute = 0, low = 0, high = 0;
	};

	struct BGShiftRegister
	{
		uint16_t low = 0, high = 0;
	};

	struct ObjectShiftRegister
	{
		bool is_sprite0 = false;
		uint8_t attribute = 0, x_position = 0;
		uint8_t pattern_low = 0, pattern_high = 0;
	};

	class PPU
	{
		bool write_toggle = false, _frame_ended = false, odd = false;
		uint8_t fine_x_scroll = 0;
		uint16_t v = 0, t = 0;
		uint16_t cycles = 0;
		uint16_t scanline_num = 0;
		FetcherState fetcher;
		BGShiftRegister bg_pixels, bg_attributes;
		std::vector<ObjectShiftRegister> oam_shifters{};
		Core &core;

	public:
		uint8_t ctrl = 0, mask = 0, status = PPUStatusFlags::VBlank | PPUStatusFlags::SpriteOverflow;
		uint8_t oam_address = 0;
		uint8_t address = 0, data = 0;
		uint32_t frame_num = 0;
		std::array<uint8_t, 0x20> palette_memory{};
		std::array<uint8_t, 0x100> oam{};
		std::array<uint32_t, 256 * 240> framebuffer{};

		PPU(Core &core);
		void reset();

		void step();
		void sprite_eval();
		void run_fetcher();
		void increment_x();
		void increment_y();
		void copy_x();
		void copy_y();
		void increment_vram();
		bool rendering_enabled() const;
		bool frame_ended();

		uint8_t cpu_read(uint16_t address);
		void cpu_write(uint16_t address, uint16_t value);
		std::tuple<uint8_t, bool> ppu_read(uint16_t address);
		uint8_t ppu_read_v(uint16_t addr);
		void ppu_write(uint16_t address, uint8_t value);
	};
}