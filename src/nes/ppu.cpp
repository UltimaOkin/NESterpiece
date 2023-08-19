#include "ppu.hpp"
#include "core.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	void PPU::step(Core &core)
	{
		if (scanline_num == 241 && cycles == 1)
		{
			status |= PPUStatusFlags::VBlank;
			core.cpu.nmi_ready = true;
		}
		else if (scanline_num == 261 && cycles == 1)
		{
			status &= ~(PPUStatusFlags::VBlank | PPUStatusFlags::Sprite0Hit | PPUStatusFlags::SpriteOverflow);
		}

		cycles++;

		if (cycles == 342)
		{
			cycles = 0;
			scanline_num++;
			scanline_num = scanline_num % 262;
		}
	}

	void PPU::increment_vram()
	{
		if (ctrl & CtrlFlags::AddressIncrement)
			v += 32;
		else
			v++;
	}

	uint8_t PPU::cpu_read(uint16_t address)
	{
		switch (address)
		{
		case 0x2002:
		{
			auto out_status = status;
			status &= ~PPUStatusFlags::VBlank;
			write_toggle = false;
			return out_status;
		}
		case 0x2004:
		{
			return oam_data;
		}

		case 0x2007:
		{
			increment_vram();
			return data;
		}
		}
		return 0;
	}

	void PPU::cpu_write(uint16_t address, uint16_t value)
	{
		switch (address)
		{
		case 0x2000:
		{
			ctrl = value;
			t &= ~VramMask::NametableSelect;
			t |= (value & 0b11) << 10;
			break;
		}
		case 0x2001:
		{
			mask = value;
			break;
		}
		case 0x2003:
		{
			oam_address = value;
			break;
		}
		case 0x2004:
		{
			oam_data = value;
			break;
		}
		case 0x2005:
		{
			if (write_toggle)
			{
				t &= ~(VramMask::FineY | VramMask::CoarseY);
				t |= (value & 0b111) << 12;
				t |= (value & 0b11111000) << 5;
			}
			else
			{
				t &= ~VramMask::CoarseX;
				t |= (value & 0b11111000) >> 3;
				fine_x_scroll = value & 0b111;
			}

			write_toggle = !write_toggle;
			break;
		}
		case 0x2006:
		{
			if (write_toggle)
			{
				constexpr uint16_t m = ~0b11111111;
				t &= ~0b11111111;
				t |= (value & 0b11111111);
				v = t;
			}
			else
			{
				t &= ~0b111111100000000;
				t |= (value & 0b111111) << 8;
			}
			write_toggle = !write_toggle;
			break;
		}
		case 0x2007:
		{
			data = value;
			increment_vram();
			break;
		}
		}
	}
}