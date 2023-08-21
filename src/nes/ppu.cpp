#include "ppu.hpp"
#include "core.hpp"
#include "cartridge.hpp"
#include "constants.hpp"
#include <cassert>

namespace NESterpiece
{
	void PPU::step()
	{
		if (within_range<uint16_t>(scanline_num, 0, 239) || (scanline_num == 261))
		{
			if (rendering_enabled())
			{
				if (within_range<uint16_t>(cycles, 1, 256) || within_range<uint16_t>(cycles, 321, 336))
					run_fetcher();

				if (cycles == 256)
					increment_y();

				if (cycles == 257)
					copy_x();

				if (within_range<uint16_t>(cycles, 280, 304) && (scanline_num == 261))
					copy_y();

				if (scanline_num < 240 && within_range<uint16_t>(cycles, 1, 256))
				{
					const uint8_t pixel_low = (bg_pixels.low >> (15 - fine_x_scroll)) & 1;
					const uint8_t pixel_high = (bg_pixels.high >> (15 - fine_x_scroll)) & 1;
					const uint8_t pixel = (pixel_high << 1) | pixel_low;

					const uint8_t attribute_low = (bg_attributes.low >> (15 - fine_x_scroll)) & 1;
					const uint8_t attribute_high = (bg_attributes.high >> (15 - fine_x_scroll)) & 1;
					const uint8_t pixel_attribute = (attribute_high << 1) | attribute_low;

					uint8_t lookup = ppu_read_v((0x3F00 + (pixel_attribute << 2) + pixel));
					if (pixel == 0)
						lookup = ppu_read_v(0x3F00);

					framebuffer[(scanline_num * 256) + (cycles - 1)] = Palette2C02[lookup];
				}
			}
		}

		if (scanline_num == 241 && cycles == 1)
		{
			status |= PPUStatusFlags::VBlank;
			_vblank_started = true;
			if (ctrl & CtrlFlags::EnableNMI)
				core.cpu.nmi_ready = true;
		}

		if (scanline_num == 261 && cycles == 1)
		{
			status &= ~PPUStatusFlags::VBlank;
		}

		if (cycles == 341)
		{
			cycles = 0;
			scanline_num = ++scanline_num % 262;
		}
		else
		{
			cycles++;
		}
	}

	void PPU::run_fetcher()
	{
		bg_pixels.low <<= 1;
		bg_pixels.high <<= 1;
		bg_attributes.low <<= 1;
		bg_attributes.high <<= 1;

		switch ((cycles - 1) & 7)
		{
		case 0:
		{
			bg_pixels.low |= fetcher.low;
			bg_pixels.high |= fetcher.high;

			// fill shift register with the same attribute bits for all pixels
			bg_attributes.low |= fetcher.tile_attribute & 0b1 ? 0xFF : 0x0;
			bg_attributes.high |= fetcher.tile_attribute & 0b10 ? 0xFF : 0x0;
			fetcher.nametable_tile = ppu_read_v(0x2000 | (v & 0x0FFF));
			break;
		}
		case 2:
		{
			const uint8_t attribute = ppu_read_v(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
			const uint8_t row = ((v & VramMask::CoarseY) >> 4) & 0x4;
			const uint8_t column = (v & VramMask::CoarseX) & 0x2;

			// if row 1 is selected then shift the bottom tiles up front
			// then select between column 1 or 2 and mask the result
			fetcher.tile_attribute = (attribute >> (row | column)) & 0x3;
			break;
		}
		case 4:
		{
			const uint16_t address = ((ctrl & CtrlFlags::BGPatternAddress) << 8) + ((uint16_t)fetcher.nametable_tile << 4) | ((v & VramMask::FineY) >> 12);
			fetcher.low = ppu_read_v(address);
			break;
		}
		case 6:
		{
			const uint16_t address = ((ctrl & CtrlFlags::BGPatternAddress) << 8) + ((uint16_t)fetcher.nametable_tile << 4) | ((v & VramMask::FineY) >> 12);
			fetcher.high = ppu_read_v(address + 8);
			break;
		}
		case 7:
		{
			increment_x();
			break;
		}
		}
	}

	// from https://www.nesdev.org/wiki/PPU_scrolling
	void PPU::increment_x()
	{
		if ((v & 0x001F) == 31) // if coarse X == 31
		{
			v &= ~0x001F; // coarse X = 0
			v ^= 0x0400;  // switch horizontal nametable
		}
		else
		{
			v += 1; // increment coarse X
		}
	}

	// from https://www.nesdev.org/wiki/PPU_scrolling
	void PPU::increment_y()
	{
		if ((v & 0x7000) != 0x7000) // if fine Y < 7
		{
			v += 0x1000; // increment fine Y
		}
		else
		{
			v &= ~0x7000;					// fine Y = 0
			uint16_t y = (v & 0x03E0) >> 5; // let y = coarse Y

			if (y == 29)
			{
				y = 0;		 // coarse Y = 0
				v ^= 0x0800; // switch vertical nametable
			}
			else if (y == 31)
			{
				y = 0; // coarse Y = 0, nametable not switched
			}
			else
			{
				y += 1; // increment coarse Y
			}
			v = (v & ~0x03E0) | (y << 5); // put coarse Y back into v
		}
	}

	void PPU::copy_x()
	{
		v &= ~(VramMask::CoarseX | VramMask::NametableX);
		v |= t & (VramMask::CoarseX | VramMask::NametableX);
	}

	void PPU::copy_y()
	{
		v &= ~(VramMask::CoarseY | VramMask::NametableY | VramMask::FineY);
		v |= t & (VramMask::CoarseY | VramMask::NametableY | VramMask::FineY);
	}

	void PPU::increment_vram()
	{
		if (ctrl & CtrlFlags::VramIncrement)
			v += 32;
		else
			v++;
	}

	bool PPU::rendering_enabled() const
	{
		return (mask & MaskFlags::ShowBG) || (mask & MaskFlags::ShowSprites);
	}

	bool PPU::vblank_started()
	{
		bool result = _vblank_started;
		_vblank_started = false;
		return result;
	}

	uint8_t PPU::cpu_read(uint16_t address)
	{
		switch (address & 7)
		{
		case 0x1:
		{
			return mask;
		}
		case 0x2:
		{
			uint16_t out_status = status;
			status &= ~PPUStatusFlags::VBlank;
			write_toggle = false;
			return out_status;
		}
		case 0x4:
		{
			return oam_data;
		}
		case 0x7:
		{
			uint8_t output = data;
			auto [read_data, should_stall] = ppu_read(v);

			if (!should_stall)
				output = read_data;
			increment_vram();
			data = read_data;
			return output;
		}
		}

		return 0;
	}

	void PPU::cpu_write(uint16_t address, uint16_t value)
	{
		switch (address & 7)
		{
		case 0x0:
		{
			ctrl = value;
			t = (t & ~VramMask::NametableSelect) | ((value & 0b11) << 10);
			return;
		}
		case 0x1:
		{
			mask = value;
			return;
		}
		case 0x3:
		{
			oam_address = value;
			return;
		}
		case 0x4:
		{
			oam_data = value;
			return;
		}
		case 0x5:
		{
			if (write_toggle)
			{
				t = (t & ~0b111001111100000) | (((value & 0b111) << 12) | ((value >> 3) << 5));
			}
			else
			{
				fine_x_scroll = value & 0b111;
				t = (t & ~0b11111) | (value >> 3);
			}

			write_toggle = !write_toggle;
			return;
		}
		case 0x6:
		{
			if (write_toggle)
			{
				t = (t & 0b1111111100000000) | value;
				v = t;
			}
			else
			{
				t = (t & 0b11111111) | ((value & 0b111111) << 8);

				// https://www.nesdev.org/wiki/PPU_scrolling#Single_scroll mentions this
				ctrl &= ~CtrlFlags::NametableSelectCtrl;
				ctrl |= ((t & VramMask::NametableSelect) >> 10) & 0b11;
			}
			write_toggle = !write_toggle;
			return;
		}
		case 0x7:
		{
			data = value;
			ppu_write(v, value);
			increment_vram();
			return;
		}
		}
	}

	std::tuple<uint8_t, bool> PPU::ppu_read(uint16_t address)
	{
		if (within_range<uint16_t>(address, 0x0000, 0x1FFF))
		{
			return {core.bus.cart->read_chr(address), true};
		}
		else if (within_range<uint16_t>(address, 0x2000, 0x3EFF))
		{
			return {core.bus.cart->read_nametable(address & 0x0FFF), true};
		}
		else if (within_range<uint16_t>(address, 0x3F00, 0x3F1F))
		{
			switch (address & 0xFF)
			{
			case 0:
			case 0x10:
				return {palette_memory[0x0], false};
			case 0x4:
			case 0x14:
				return {palette_memory[0x4], false};
			case 0x8:
			case 0x18:
				return {palette_memory[0x8], false};
			case 0xC:
			case 0x1C:
				return {palette_memory[0xC], false};
			default:
				return {palette_memory[address & 0xFF], false};
			}
		}

		return {0, false};
	}

	uint8_t PPU::ppu_read_v(uint16_t addr)
	{
		auto [v, _] = ppu_read(addr);
		return v;
	}

	void PPU::ppu_write(uint16_t address, uint8_t value)
	{
		if (within_range<uint16_t>(address, 0x0000, 0x1FFF))
		{
			core.bus.cart->write_chr(address, value);
		}
		else if (within_range<uint16_t>(address, 0x2000, 0x3EFF))
		{
			core.bus.cart->write_nametable(address & 0x0FFF, value);
		}
		else if (within_range<uint16_t>(address, 0x3F00, 0x3F1F))
		{
			switch (address & 0xFF)
			{
			case 0:
			case 0x10:
				palette_memory[0x0] = value;
				break;
			case 0x4:
			case 0x14:
				palette_memory[0x4] = value;
				break;
			case 0x8:
			case 0x18:
				palette_memory[0x8] = value;
				break;
			case 0xC:
			case 0x1C:
				palette_memory[0xC] = value;
				break;
			default:
				palette_memory[address & 0xFF] = value;
				break;
			}
		}
	}
}