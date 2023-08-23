#include "pad.hpp"

namespace NESterpiece
{
	void StdController::set_button_state(StdControllerButton btn, bool pressed)
	{
		data_line &= ~btn;
		data_line |= pressed ? btn : 0;
	}

	uint8_t StdController::read_and_shift()
	{
		uint8_t out = data_line & 1;
		data_line >>= 1;
		return out;
	}
}
