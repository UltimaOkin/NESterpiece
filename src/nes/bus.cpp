#include "bus.hpp"
#include "cpu.hpp"
#include "cartridge.hpp"
#include "ppu.hpp"
#include "oam.hpp"
#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{

	uint8_t Bus::read(uint16_t address)
	{
		core.tick_components(true);
		activity = {
			.value = 0,
			.address = address,
			.type = BusActivityType::Read,
		};

		return read_no_tick(address);
	}

	void Bus::write(uint16_t address, uint8_t value)
	{
		core.tick_components(false);
		activity = {
			.value = value,
			.address = address,
			.type = BusActivityType::Write,
		};

		write_no_tick(address, value);
	}

	uint8_t Bus::read_no_tick(uint16_t address)
	{
		if (within_range<uint16_t>(address, 0, 0x7FF) || within_range<uint16_t>(address, 0x800, 0xFFF) || within_range<uint16_t>(address, 0x1000, 0x17FF) || within_range<uint16_t>(address, 0x1800, 0x1FFF))
		{
			return activity.value = internal_ram[address & 0x7FF];
		}
		else if (within_range<uint16_t>(address, 0x2000, 0x3FFF))
		{
			return activity.value = ppu.cpu_read(address);
		}
		else if (within_range<uint16_t>(address, 0x4000, 0x4017))
		{
			switch (address & 0xFF)
			{
			case 0x16:
				return pad.read_and_shift();
			default:
				return 0;
			}
		}
		else if (within_range<uint16_t>(address, 0x4020, 0xFFFF))
		{
			uint16_t base = address;
			return activity.value = cart->read(address);
		}

		return 0;
	}
	void Bus::write_no_tick(uint16_t address, uint8_t value)
	{

		if (within_range<uint16_t>(address, 0, 0x7FF) ||
			within_range<uint16_t>(address, 0x800, 0xFFF) ||
			within_range<uint16_t>(address, 0x1000, 0x17FF) ||
			within_range<uint16_t>(address, 0x1800, 0x1FFF))
		{
			internal_ram[address & 0x7FF] = value;
		}
		else if (within_range<uint16_t>(address, 0x2000, 0x3FFF))
		{
			ppu.cpu_write(address, value);
		}
		else if (within_range<uint16_t>(address, 0x4000, 0x4017))
		{
			switch (address & 0xFF)
			{
			case 0x14:
				oam_dma.start(value);
				break;
			case 0x16:
				pad.write(value);
				break;
			}
		}
		else if (within_range<uint16_t>(address, 0x4020, 0xFFFF))
		{
			cart->write(address, value);
		}
	}
}
