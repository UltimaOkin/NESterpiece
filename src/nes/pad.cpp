#include "pad.hpp"

namespace NESterpiece
{
	void StdController::reset()
	{
		data_line = 0;
	}

	void StdController::set_button_state(StdControllerButton btn, bool pressed)
	{
		data_line &= ~btn;
		data_line |= pressed ? btn : 0;
	}

	void StdController::capture_state()
	{
		if (active)
		{
			if (input_poll_cb)
				input_poll_cb(*this);

			captured = data_line;
		}
	}

	void StdController::write(uint8_t value)
	{
		active = value & 1 ? true : false;
		capture_state();
	}

	uint8_t StdController::read_and_shift()
	{
		uint8_t out = captured & 1;
		captured >>= 1;
		return out;
	}
}
