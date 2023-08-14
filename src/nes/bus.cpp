#include "bus.hpp"
#include "constants.hpp"
namespace NESterpiece
{

	uint8_t Bus::read(uint16_t address)
	{
		if (within_range(address, 0, 0x7FF) ||
			within_range(address, 0x800, 0xFFF) ||
			within_range(address, 0x1000, 0x17FF) ||
			within_range(address, 0x1800, 0x1FFF))
		{
			return internal_ram[address & 0x7FF];
		}
		else if (within_range(address, 0x2000, 0x3FFF))
		{
			uint16_t base = address & 0x7;
			// PPU regs
			return 0;
		}
		else if (within_range(address, 0x4000, 0x4017))
		{
			uint16_t base = address & 0x1F;
		}
		else if (within_range(address, 0x4020, 0xFFFF))
		{
			// cart
			uint16_t base = address;
		}

		return 0xFF;
	}

	void Bus::write(uint16_t address, uint8_t value)
	{
	}
}
