#pragma once
#include <cinttypes>

namespace NESterpiece
{
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

	enum StatusFlags
	{
		Overflow = 32,
		Sprite0Hit = 64,
		VBlank = 128,
	};

	class PPU
	{
	public:
		uint8_t ctrl = 0, mask = 0, status = 0;
		uint8_t oam_address = 0, oam_data = 0;
		uint8_t scroll_x = 0, scroll_y = 0;
		uint8_t address = 0, data = 0;
		uint8_t dma_address = 0;
	};
}