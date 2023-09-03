#pragma once
#include <cinttypes>
#include <array>
namespace NESterpiece
{
	constexpr int32_t SCREEN_WIDTH = 256;
	constexpr int32_t SCREEN_HEIGHT = 240;
	constexpr int8_t CPU_CLOCK_DIVIDER = 12;
	constexpr int8_t PPU_CLOCK_DIVIDER = 4;
	constexpr uint16_t NMI_VECTOR_START = 0xFFFA;
	constexpr uint16_t RESET_VECTOR_START = 0xFFFC;
	constexpr uint16_t IRQ_VECTOR_START = 0xFFFE;
	constexpr uint16_t BRK_VECTOR_START = IRQ_VECTOR_START;

	// From: https://www.nesdev.org/wiki/PPU_palettes
	constexpr std::array<uint32_t, 64> Palette2C02{
		0x626262FF,
		0x001fb2FF,
		0x2404c8FF,
		0x5200b2FF,
		0x730076FF,
		0x800024FF,
		0x730b00FF,
		0x522800FF,
		0x244400FF,
		0x005700FF,
		0x005c00FF,
		0x005324FF,
		0x003c76FF,
		0x000000FF,
		0x000000FF,
		0x000000FF,

		0xabababFF,
		0x0d57ffFF,
		0x4b30ffFF,
		0x8a13ffFF,
		0xbc08d6FF,
		0xd21269FF,
		0xc72e00FF,
		0x9d5400FF,
		0x607b00FF,
		0x209800FF,
		0x00a300FF,
		0x009942FF,
		0x007db4FF,
		0x000000FF,
		0x000000FF,
		0x000000FF,

		0xffffffFF,
		0x53aeffFF,
		0x9085ffFF,
		0xd365ffFF,
		0xff57ffFF,
		0xff5dcfFF,
		0xff7757FF,
		0xfa9e00FF,
		0xbdc700FF,
		0x7ae700FF,
		0x43f611FF,
		0x26ef7eFF,
		0x2cd5f6FF,
		0x4e4e4eFF,
		0x000000FF,
		0x000000FF,

		0xffffffFF,
		0xb6e1ffFF,
		0xced1ffFF,
		0xe9c3ffFF,
		0xffbcffFF,
		0xffbdf4FF,
		0xffc6c3FF,
		0xffd59aFF,
		0xe9e681FF,
		0xcef481FF,
		0xb6fb9aFF,
		0xa9fac3FF,
		0xa9f0f4FF,
		0xb8b8b8FF,
		0x000000FF,
		0x000000FF};

	template <class T>
	constexpr bool within_range(T value, T start, T end)
	{
		return ((value >= start) && (value <= end));
	}
}