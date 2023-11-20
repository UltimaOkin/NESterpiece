#include "ppu.hpp"
#include "core.hpp"
#include "cartridge.hpp"
#include "constants.hpp"
#include <cassert>

namespace NESterpiece
{
	PPU::PPU(Core &core) : core(core)
	{
	}

	void PPU::reset()
	{
		write_toggle = _frame_ended = odd = false;
		fine_x_scroll = 0;
		v = t = cycles = scanline_num = 0;
		fetcher = FetcherState{};
		bg_pixels = BGShiftRegister{};
		bg_attributes = BGShiftRegister{};
		oam_shifters.clear();

		ctrl = mask = 0;
		status = PPUStatusFlags::VBlank | PPUStatusFlags::SpriteOverflow;
		oam_address = address = data = 0;
		frame_num = 0;
		palette_memory.fill(0);
		oam.fill(0);
		framebuffer.fill(0);
		w2006_cycles = 0;
		w2006_delay = false;
	}

	void PPU::update_snapshot()
	{
		snapshot.cycles = cycles;
		snapshot.frame_num = frame_num;
		snapshot.total_frame_cycles = total_frame_cycles;
		snapshot.scanline = scanline_num;
		snapshot.v = v;
		snapshot.t = t;
		snapshot.fine_x = fine_x_scroll;
		snapshot.write_toggle = write_toggle;
		snapshot.control = ctrl;
		snapshot.status = status;
		snapshot.mask = mask;
		snapshot.oam_address = oam_address;
	}

	void PPU::step()
	{
		if (within_range<uint16_t>(scanline_num, 0, 239) || (scanline_num == 261))
		{
			if (rendering_enabled())
			{

				if (within_range<uint16_t>(cycles, 1, 256) || within_range<uint16_t>(cycles, 321, 336))
					run_fetcher();

				if (cycles == 256)
				{
					increment_y();
				}

				if (cycles == 257)
					copy_x();

				if (within_range<uint16_t>(cycles, 280, 304) && (scanline_num == 261))
					copy_y();

				if (scanline_num < 240 && within_range<uint16_t>(cycles, 1, 256))
				{
					const uint8_t pixel_low = (bg_pixels.low >> (15 - fine_x_scroll)) & 1;
					const uint8_t pixel_high = (bg_pixels.high >> (15 - fine_x_scroll)) & 1;
					const uint8_t bg_pixel = (pixel_high << 1) | pixel_low;

					const uint8_t attribute_low = (bg_attributes.low >> (15 - fine_x_scroll)) & 1;
					const uint8_t attribute_high = (bg_attributes.high >> (15 - fine_x_scroll)) & 1;
					const uint8_t pixel_attribute = (attribute_high << 1) | attribute_low;

					uint8_t lookup = ppu_read_v((0x3F00 + (pixel_attribute << 2) + bg_pixel));
					if (bg_pixel == 0)
						lookup = ppu_read_v(0x3F00);

					const uint16_t x_pos = cycles - 1;
					bool check_sprite0 = x_pos != 255;

					if ((mask & MaskFlags::ShowBGOnLeft) == 0)
					{
						if (x_pos > 7)
						{
							framebuffer[(scanline_num * 256) + x_pos] = Palette2C02[lookup];
						}
						else
						{
							framebuffer[(scanline_num * 256) + x_pos] = Palette2C02[0];
							check_sprite0 = false;
						}
					}
					else
					{
						if (mask & MaskFlags::ShowBG)
							framebuffer[(scanline_num * 256) + x_pos] = Palette2C02[lookup];
						else
							framebuffer[(scanline_num * 256) + x_pos] = Palette2C02[0];
					}

					if (mask & MaskFlags::ShowSprites)
					{
						for (auto &shifter : oam_shifters)
						{
							if (shifter.x_position > 0)
							{
								shifter.x_position--;
							}
							else
							{
								const uint8_t pixel_attribute = shifter.attribute & 0x3;
								uint8_t pixel_low = 0;
								uint8_t pixel_high = 0;

								if (shifter.attribute & ObjectAttribute::FlipX)
								{
									pixel_low = shifter.pattern_low & 1;
									pixel_high = shifter.pattern_high & 1;
									shifter.pattern_low >>= 1;
									shifter.pattern_high >>= 1;
								}
								else
								{
									pixel_low = (shifter.pattern_low >> 7) & 1;
									pixel_high = (shifter.pattern_high >> 7) & 1;
									shifter.pattern_low <<= 1;
									shifter.pattern_high <<= 1;
								}

								const uint8_t pixel = (pixel_high << 1) | pixel_low;

								uint8_t lookup = ppu_read_v((0x3F10 + (pixel_attribute << 2) + pixel));

								if (((mask & MaskFlags::ShowSpritesOnLeft) == 0) && (x_pos < 8))
									continue;

								if (pixel > 0)
								{
									if (bg_pixel > 0 && shifter.is_sprite0 && x_pos >= 2)
										status |= PPUStatusFlags::Sprite0Hit;

									if ((shifter.attribute & ObjectAttribute::Priority) == 0 || bg_pixel == 0)
										framebuffer[(scanline_num * 256) + (cycles - 1)] = Palette2C02[lookup];
								}
							}
						}
					}
				}
			}
		}

		if (cycles == 256 && within_range<uint16_t>(scanline_num, 0, 239))
		{
			sprite_eval();
		}

		if (scanline_num == 241 && cycles == 1)
		{
			status |= PPUStatusFlags::VBlank;
			_frame_ended = true;
			if (ctrl & CtrlFlags::EnableNMI)
				core.cpu.nmi_ready = true;
		}

		if (scanline_num == 261 && cycles == 1)
		{
			oam_shifters.clear();
			status &= ~(PPUStatusFlags::VBlank | PPUStatusFlags::Sprite0Hit | PPUStatusFlags::SpriteOverflow);
		}

		check_snapshots();

		if (w2006_delay && w2006_cycles == 3)
		{
			v = t;
			w2006_delay = false;
		}
		else
		{
			w2006_cycles++;
		}

		if (cycles == 339 && scanline_num == 261 && odd && rendering_enabled())
			cycles++;

		if (cycles == 340)
		{
			cycles = 0;
			scanline_num = ++scanline_num % 262;
			if (scanline_num == 0)
			{
				odd = !odd;
				total_frame_cycles = 0;
				frame_num++;
			}
		}
		else
		{
			total_frame_cycles++;
			cycles++;
		}
	}

	void PPU::check_snapshots()
	{
		switch (update_event)
		{
		case SnapshotEvent::OnFrame:
		{
			if (frame_num == trigger_frame)
			{
				update_snapshot();
			}
			break;
		}
		case SnapshotEvent::OnScanlineCycle:
		{
			if (scanline_num == trigger_scanline && cycles == trigger_cycle)
			{
				update_snapshot();
			}
			break;
		}
		}
	}

	void PPU::sprite_eval()
	{
		const uint16_t scanline = scanline_num;
		const bool large_sprites = ctrl & CtrlFlags::OAMSize;
		const uint16_t height = large_sprites ? 16 : 8;

		oam_shifters.clear();
		for (size_t i = 0, num_objects = 0; i < oam.size() / 4; ++i)
		{

			const uint16_t y_pos = oam[i * 4];

			if (y_pos <= scanline && (y_pos + height) > scanline)
			{
				if (oam_shifters.size() == 8)
				{
					status |= PPUStatusFlags::SpriteOverflow;
					break;
				}

				ObjectShiftRegister shifter{
					.is_sprite0 = i == 0,
					.attribute = oam[(i * 4) + 2],
					.x_position = oam[(i * 4) + 3],
					.pattern_low = 0,
					.pattern_high = 0,
				};

				uint8_t tile = oam[(i * 4) + 1];
				uint8_t pattern_table = ctrl & CtrlFlags::OAMPatternAddress;

				if (large_sprites)
				{
					pattern_table = tile & 1;
					tile = tile & (~1);
				}

				if (shifter.attribute & ObjectAttribute::FlipY)
				{
					shifter.pattern_low = core.bus.cart->read_chr(pattern_table, tile, 0, (height - 1) - (scanline - y_pos));
					shifter.pattern_high = core.bus.cart->read_chr(pattern_table, tile, 1, (height - 1) - (scanline - y_pos));
				}
				else
				{
					shifter.pattern_low = core.bus.cart->read_chr(pattern_table, tile, 0, scanline - y_pos);
					shifter.pattern_high = core.bus.cart->read_chr(pattern_table, tile, 1, scanline - y_pos);
				}

				oam_shifters.push_back(std::move(shifter));
			}
		}
	}

	void PPU::run_fetcher()
	{
		bg_pixels.low <<= 1;
		bg_pixels.high <<= 1;
		bg_attributes.low <<= 1;
		bg_attributes.high <<= 1;

		switch (cycles & 7)
		{
		case 1:
		{
			bg_pixels.low |= fetcher.low;
			bg_pixels.high |= fetcher.high;

			// fill shift register with the same attribute bits for all pixels
			bg_attributes.low |= fetcher.tile_attribute & 0b1 ? 0xFF : 0x0;
			bg_attributes.high |= fetcher.tile_attribute & 0b10 ? 0xFF : 0x0;
			fetcher.nametable_tile = ppu_read_v(0x2000 | (v & 0x0FFF));
			break;
		}
		case 3:
		{
			const uint8_t attribute = ppu_read_v(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
			const uint8_t row = ((v & VramMask::CoarseY) >> 4) & 0x4;
			const uint8_t column = (v & VramMask::CoarseX) & 0x2;

			// if row 1 is selected then shift the bottom tiles up front
			// then select between column 1 or 2 and mask the result
			fetcher.tile_attribute = (attribute >> (row | column)) & 0x3;
			break;
		}
		case 5:
		{
			const auto pattern_table = static_cast<uint16_t>(ctrl & CtrlFlags::BGPatternAddress) << 8;
			const auto tile = static_cast<uint16_t>(fetcher.nametable_tile) << 4;
			const uint16_t fine_y = (v & VramMask::FineY) >> 12;
			fetcher.low = ppu_read_v(pattern_table + tile | fine_y);
			break;
		}
		case 7:
		{
			const auto pattern_table = static_cast<uint16_t>(ctrl & CtrlFlags::BGPatternAddress) << 8;
			const auto tile = static_cast<uint16_t>(fetcher.nametable_tile) << 4;
			const uint16_t fine_y = (v & VramMask::FineY) >> 12;
			fetcher.high = ppu_read_v((pattern_table + tile | fine_y) + 8);
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

	bool PPU::frame_ended()
	{
		bool result = _frame_ended;
		_frame_ended = false;
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
			return oam[oam_address];
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

			if ((value & CtrlFlags::EnableNMI) && ((ctrl & CtrlFlags::EnableNMI) == 0))
			{
				if (status & PPUStatusFlags::VBlank)
					core.cpu.nmi_ready = true;
			}
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
			if (frame_num == 4)
			{
				int dummy = 0;
			}
			oam[oam_address] = value;
			oam_address++;
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
				w2006_delay = true;
				w2006_cycles = 0;
				// v = t;
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