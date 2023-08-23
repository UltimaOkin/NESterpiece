#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	Core::Core()
		: cpu(), ppu(*this), bus(ppu, cpu.oam_dma)
	{
	}

	void Core::reset(std::shared_ptr<Cartridge> cart)
	{
		cpu.reset();
		ppu.reset();
		bus.cart = std::move(cart);
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

				// if oamdma is running then the CPU does nothing
				cpu.oam_dma.step(bus, ppu);
				if (!cpu.oam_dma.active)
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
				cpu.oam_dma.step(bus, ppu);
				if (!cpu.oam_dma.active)
					cpu.step(bus);
			}

		} while (!ppu.frame_ended());
	}
}