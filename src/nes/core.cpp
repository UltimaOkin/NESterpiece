#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	Core::Core()
		: cpu(), ppu(*this), bus(ppu, cpu.oam_dma, *this)
	{
	}

	void Core::reset(std::shared_ptr<Cartridge> cart)
	{
		bus.cart = std::move(cart);
		cpu.reset();
		ppu.reset();
	}

	void Core::tick_components(bool read_cycle)
	{
		if (read_cycle)
		{
			if (cpu.oam_dma.active)
			{
				while (cpu.oam_dma.active)
				{
					for (uint32_t i = 0; i < 3; ++i)
					{
						ppu.step();
					}
					cpu.oam_dma.step(bus, ppu);
				}
			}
			else
			{
				for (uint32_t i = 0; i < 3; ++i)
				{
					ppu.step();
				}
			}
		}
		else
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				ppu.step();
			}
		}
	}

	void Core::tick_until_vblank()
	{
		do
		{
			cpu.step(bus);

		} while (!ppu.frame_ended());
	}
}