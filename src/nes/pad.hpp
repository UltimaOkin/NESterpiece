#pragma once
#include <cinttypes>
#include <functional>
namespace NESterpiece
{
	// Status for each controller is returned as an 8-bit report in the following order: A, B, Select, Start, Up, Down, Left, Right.

	enum StdControllerButton
	{
		A = 1,
		B = 2,
		Select = 4,
		Start = 8,
		Up = 16,
		Down = 32,
		Left = 64,
		Right = 128
	};

	class StdController
	{
		uint8_t captured = 0;
		uint8_t data_line = 0;

	public:
		bool active = false;
		std::function<void(StdController &)> input_poll_cb;
		void reset();
		void set_button_state(StdControllerButton btn, bool pressed);
		void capture_state();

		void write(uint8_t value);
		uint8_t read_and_shift();
	};
}