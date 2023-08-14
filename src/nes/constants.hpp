#pragma once
#include <cinttypes>

namespace NESterpiece
{
	constexpr int8_t CPU_CLOCK_DIVIDER = 12;
	constexpr int8_t PPU_CLOCK_DIVIDER = 4;
	constexpr uint16_t NMI_VECTOR_START = 0xFFFA;
	constexpr uint16_t RESET_VECTOR_START = 0xFFFC;
	constexpr uint16_t IRQ_VECTOR_START = 0xFFFE;
	constexpr uint16_t BRK_VECTOR_START = IRQ_VECTOR_START;

	constexpr bool within_range(uint16_t address, uint16_t start, uint16_t end)
	{
		return ((address >= start) && (address <= end));
	}
}