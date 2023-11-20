#include "diagnostics.hpp"
#include "../state.hpp"
#include "../gui_constants.hpp"
#include <nes/core.hpp>
#include <imgui.h>
#include <fmt/format.h>
namespace NESterpiece
{
	void PPUDiagnostics::draw(EmulationState &state)
	{
		using namespace ImGui;

		SetNextWindowBgAlpha(0.6f);
		if (Begin("PPU Registers", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static int32_t v[2]{0, 0};
			SetNextItemWidth(100);
			uint16_t step = 1;
			uint16_t stepf = 10;
			InputScalar("Refresh on Scanline", ImGuiDataType_U16, &state.core.ppu.trigger_scanline, &step, &stepf);
			SetNextItemWidth(100);
			InputScalar("Refresh on Cycle", ImGuiDataType_U16, &state.core.ppu.trigger_cycle, &step, &stepf);

			if (BeginTable("PPU Registers Tbl", 2, ImGuiTableFlags_SizingFixedFit, ImVec2(320, 0)))
			{
				const auto &ppu = state.core.ppu.snapshot;
				{
					TableNextColumn();
					Text("Frame Num");
					TableNextColumn();
					const auto str = fmt::format("{}", ppu.frame_num);
					Text("%s", str.c_str());
				}
				{
					TableNextRow();
					TableNextColumn();
					Text("Frame Cycle Num");
					TableNextColumn();
					const auto str = fmt::format("{}", ppu.total_frame_cycles);
					Text("%s", str.c_str());
				}

				{
					TableNextRow();
					TableNextColumn();
					Text("Scanline Num");
					TableNextColumn();
					const auto str = fmt::format("{}", ppu.scanline);
					Text("%s", str.c_str());
				}
				{
					TableNextRow();
					TableNextColumn();
					Text("Scanline Cycle Num");
					TableNextColumn();
					const auto str = fmt::format("{}", ppu.cycles);
					Text("%s", str.c_str());
				}

				{
					TableNextRow();
					TableNextColumn();
					Text("OAM Address");
					TableNextColumn();
					const auto str = fmt::format("${:02X}", ppu.oam_address);
					Text("%s", str.c_str());
				}

				draw_internal_regs(ppu.v, ppu.t, ppu.fine_x, ppu.write_toggle);
				draw_control_reg(ppu.control);
				draw_status_reg(ppu.status);
				EndTable();
			}
		}
		End();
	}

	void PPUDiagnostics::draw_internal_regs(uint16_t vram, uint16_t t, uint8_t fine_x, bool write_toggle)
	{
		using namespace ImGui;
		TableNextRow();
		TableNextColumn();
		Spacing();
		TextColored(MENU_TEXT_DARK, "Internal Registers");
		Spacing();
		TableNextColumn();
		TableNextRow();

		{
			TableNextColumn();
			Text("VRAM Address");
			TableNextColumn();
			const auto str = fmt::format("${:04X}", vram);
			Text("%s", str.c_str());
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("VRAM (T) Address");
			TableNextColumn();
			const auto str = fmt::format("${:04X}", t);
			Text("%s", str.c_str());
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Fine X Scroll");
			TableNextColumn();
			const auto str = fmt::format("${:01X}", fine_x);
			Text("%s", str.c_str());
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Write Toggle");
			TableNextColumn();
			constexpr std::array<const char *, 2> toggle{
				"First (0)",
				"Second (1)",
			};

			Text("%s", toggle[write_toggle]);
		}
	}

	void PPUDiagnostics::draw_control_reg(uint8_t control)
	{
		using namespace ImGui;
		TableNextRow();
		TableNextColumn();
		Spacing();
		TextColored(MENU_TEXT_DARK, "$2000 Control");
		Spacing();
		TableNextColumn();
		{
			TableNextRow();
			TableNextColumn();
			Text("Nametable Address");
			TableNextColumn();
			constexpr std::array<const char *, 4> nt{
				"$2000",
				"$2400",
				"$2800",
				"$2C00",
			};

			Text("%s", nt[control & CtrlFlags::NametableSelectCtrl]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("VRAM R/W Increment");
			TableNextColumn();
			constexpr std::array<const char *, 2> inc{
				"1",
				"32",
			};

			Text("%s", inc[(control & CtrlFlags::VramIncrement) >> 2]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Sprite Pattern Table");
			TableNextColumn();
			constexpr std::array<const char *, 2> pt{
				"$0000",
				"$1000",
			};

			Text("%s", pt[(control & CtrlFlags::OAMPatternAddress) >> 3]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("BG Pattern Table");
			TableNextColumn();
			constexpr std::array<const char *, 2> pt{
				"$0000",
				"$1000",
			};

			Text("%s", pt[(control & CtrlFlags::BGPatternAddress) >> 4]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Sprite Size");
			TableNextColumn();
			constexpr std::array<const char *, 2> ss{
				"8x8",
				"8x16",
			};

			Text("%s", ss[(control & CtrlFlags::OAMSize) >> 5]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("PPU EXT Mode");
			TableNextColumn();
			constexpr std::array<const char *, 2> ext{
				"Read from EXT pins",
				"Write to EXT pins",
			};

			Text("%s", ext[(control & CtrlFlags::ExtPinMode) >> 6]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Enable NMI");
			TableNextColumn();
			constexpr std::array<const char *, 2> ss{
				"False",
				"True",
			};

			Text("%s", ss[(control & CtrlFlags::EnableNMI) >> 7]);
		}
	}

	void PPUDiagnostics::draw_status_reg(uint8_t status)
	{
		using namespace ImGui;
		TableNextRow();
		TableNextColumn();
		Spacing();
		TextColored(MENU_TEXT_DARK, "$2001 Status");
		Spacing();
		TableNextColumn();
		{
			TableNextRow();
			TableNextColumn();
			Text("Sprite Overflow");
			TableNextColumn();
			constexpr std::array<const char *, 2> status_result{
				"False",
				"True",
			};

			Text("%s", status_result[(status & PPUStatusFlags::SpriteOverflow) >> 5]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("Sprite Zero Hit");
			TableNextColumn();
			constexpr std::array<const char *, 2> status_result{
				"False",
				"True",
			};

			Text("%s", status_result[(status & PPUStatusFlags::Sprite0Hit) >> 6]);
		}

		{
			TableNextRow();
			TableNextColumn();
			Text("VBlank Period");
			TableNextColumn();
			constexpr std::array<const char *, 2> status_result{
				"False",
				"True",
			};

			Text("%s", status_result[(status & PPUStatusFlags::VBlank) >> 7]);
		}
	}
}