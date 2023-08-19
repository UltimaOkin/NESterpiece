#pragma once
#include <cinttypes>
#include <array>
#include <vector>
namespace NESterpiece
{
	class Core;
	enum CtrlFlags
	{
		NametableAddressBits = 3,
		AddressIncrement = 4,
		SpritePatternAddress = 8,
		BGPatternAddress = 16,
		SpriteSize = 32,
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
		FineY = 0b111 << 12,
	};

	struct TileFetcher
	{
		uint8_t step = 0;
		uint8_t nametable = 0, attribute = 0, low = 0, high = 0;
	};
	class PPU
	{
		bool write_toggle = false;
		uint8_t fine_x_scroll = 0;
		uint16_t v = 0, t = 0;
		uint16_t cycles = 0;
		TileFetcher fetcher;
		uint16_t scanline_num = 0;

	public:
		uint8_t ctrl = 0, mask = 0, status = 0;
		uint8_t oam_address = 0, oam_data = 0;
		uint8_t address = 0, data = 0, dma_address = 0;
		std::array<uint8_t, 0x800> nametables{};
		std::array<uint32_t, 256 * 240> framebuffer{};

		void step(Core &core);
		void increment_vram();
		uint8_t cpu_read(uint16_t address);
		void cpu_write(uint16_t address, uint16_t value);
	};
}