#include "oam.hpp"
#include "ppu.hpp"
#include "bus.hpp"

namespace NESterpiece
{
	void OAMDMA::start(uint8_t page)
	{
		active = true;
		address_snap = address = static_cast<uint16_t>(page) << 8;
		data = 0;
		total_cycles = 1;
		bytes_left = 256;

		if (put_cycle)
			alignment++;
	}

	void OAMDMA::step(Bus &bus, PPU &ppu)
	{
		total_cycles++;

		// dma can only be started on a get cycle so it will need to
		// align itself by waiting one cycle.
		// additionally the APU's DMA takes priority over this
		if (alignment > 0)
		{
			--alignment;
		}
		else
		{
			if (active)
			{
				if (bytes_left == 0)
				{
					active = false;
				}
				else
				{
					if (put_cycle)
					{
						ppu.cpu_write(0x2004, data);
						--bytes_left;
					}
					else
					{
						data = bus.read_no_tick(address++);
					}
				}
			}
		}

		put_cycle = !put_cycle;
	}
}