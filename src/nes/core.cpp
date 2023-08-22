#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	Core::Core()
		: cpu(), ppu(*this), bus(ppu, cpu.oam_dma)
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
				ppu.step();
			}
			if (cpu_counter == CPU_CLOCK_DIVIDER)
			{
				cpu_counter = 0;
				cpu.step(bus);
			}
		}
	}
	void Core::tick_until_vblank()
	{
		do
		{
			cpu_counter++;
			ppu_counter++;

			if (ppu_counter == PPU_CLOCK_DIVIDER)
			{
				ppu_counter = 0;
				ppu.step();
			}

			if (cpu_counter == CPU_CLOCK_DIVIDER)
			{
				cpu_counter = 0;

				// if oamdma is running then the CPU does nothing
				if (!cpu.oam_dma.active)
					cpu.step(bus);

				cpu.oam_dma.step(bus, ppu);
			}

		} while (!ppu.vblank_started());
	}
}