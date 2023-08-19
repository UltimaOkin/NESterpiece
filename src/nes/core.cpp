#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	Core::Core()
		: cpu(), ppu(), bus(ppu)
	{
	}

	void Core::tick_components(uint32_t rate)
	{
		for (uint32_t i = 0; i < rate; ++i)
		{
			cpu_counter++;
			ppu_counter++;

			if (ppu_counter == PPU_CLOCK_DIVIDER)
			{
				ppu_counter = 0;
				ppu.step(*this);
			}
			if (cpu_counter == CPU_CLOCK_DIVIDER)
			{
				cpu_counter = 0;
				cpu.step(bus);
			}
		}
	}
}