#include "bus.hpp"

namespace NESterpiece
{
	uint8_t Bus::read(uint16_t address)
	{
		last_activity = BusActivity{
			.address = address,
			.value = memory[address],
			.type = BusActivityType::Read,
		};
		return memory[address];
	}

	void Bus::write(uint16_t address, uint8_t value)
	{
		memory[address] = value;
		last_activity = BusActivity{
			.address = address,
			.value = value,
			.type = BusActivityType::Write,
		};
	}
}
