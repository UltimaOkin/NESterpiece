#include "bus.hpp"

namespace NESterpiece
{
	uint8_t Bus::read(uint16_t address) const
	{
		return memory[address];
	}

	void Bus::write(uint16_t address, uint8_t value)
	{
		memory[address] = value;
	}
}
