#pragma once
#include <cinttypes>
namespace NESterpiece
{
	class EmulationState;

	class PPUDiagnostics
	{
	public:
		void draw(EmulationState &state);
		void draw_internal_regs(uint16_t vram, uint16_t t, uint8_t fine_x, bool write_toggle);
		void draw_control_reg(uint8_t control);
		void draw_status_reg(uint8_t status);
	};
}