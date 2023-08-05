#pragma once
#include <cinttypes>

namespace NESterpiece
{
	class Bus;

	enum StatusFlags
	{
		Carry = 1,
		Zero = 2,
		IRQ = 4,
		Decimal = 8,
		Break = 16,
		Blank = 32,
		Overflow = 64,
		Negative = 128
	};

	enum class CPURegister
	{
		A,
		X,
		Y
	};

	class CPU
	{
		using cpu_function = void (CPU::*)(Bus &);
		struct ExecutionState
		{
			bool complete = false;
			uint8_t current_cycle = 0;
			uint16_t data = 0;
			uint16_t address = 0;
			cpu_function addressing_function = nullptr, operation_function = nullptr;
		};

		int8_t ticks = 0;
		ExecutionState state{
			.complete = true,
			.current_cycle = 0,
			.data = 0,
			.address = 0,
			.addressing_function = nullptr,
			.operation_function = nullptr,
		};

	public:
		struct Registers
		{
			uint16_t pc = 0;
			uint8_t a = 0, x = 0, y = 0, sp = 0xFF, sr = 0;
		} registers;

		void reset();

		void step(Bus &bus);
		void decode(uint8_t opcode);

		void adm_immediate(Bus &bus);
		void adm_zero_page_r(Bus &bus);
		void adm_zero_page_w(Bus &bus);
		void adm_zero_page_rmw(Bus &bus);
		void op_lda(Bus &bus);
	};
}