#pragma once
#include <cinttypes>

namespace NESterpiece
{
	class Bus;

	enum class AddressingMode
	{
		Immediate,
	};

	class CPU
	{
		using cpu_function = void (CPU::*)(Bus &);
		struct ExecutionState
		{
			bool complete = false;
			uint8_t current_cycle = 0;
			uint8_t fetched_data = 0;
			uint16_t fetched_address = 0;
			cpu_function addressing_function = nullptr, operation_function = nullptr;
		};

		int8_t ticks = 0;
		ExecutionState state{};

	public:
		struct Registers
		{
			uint16_t pc = 0;
			uint8_t a = 0, x = 0, y = 0, s = 0xFF, p = 0;
		} registers;

		void reset();

		void step(Bus &bus);
		void decode(uint8_t opcode);

		void immediate_addressing(Bus &bus);

		void lda(Bus &bus);
		void sta(Bus &bus);
		void stx(Bus &bus);
		void sty(Bus &bus);
	};
}