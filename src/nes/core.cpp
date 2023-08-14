#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	void Core::tick_components(uint32_t rate)
	{
		for (uint32_t i = 0; i < rate; ++i)
		{
			cpu_counter++;

			if (cpu_counter == CPU_CLOCK_DIVIDER)
			{
				cpu_counter = 0;
				cpu.step(bus);
			}
		}
	}
}