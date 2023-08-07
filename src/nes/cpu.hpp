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
	enum class InstructionType
	{
		Read,
		Write
	};

	class CPU
	{
		using cpu_function = void (CPU::*)(Bus &);
		struct ExecutionState
		{
			bool complete = false, page_crossed = false;
			uint8_t current_cycle = 0;
			uint16_t data = 0;
			uint16_t address = 0;
			cpu_function addressing_function = nullptr, operation_function = nullptr;
		};

		int8_t ticks = 0;
		ExecutionState state{
			.complete = true,
			.page_crossed = false,
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
		void op_lda(Bus &bus);
		void op_ldx(Bus &bus);
		void op_ldy(Bus &bus);
		void op_eor(Bus &bus);
		void op_and(Bus &bus);
		void op_ora(Bus &bus);
		void op_adc(Bus &bus);
		void op_sbc(Bus &bus);

		void adm_implied(Bus &bus);
		void adm_immediate(Bus &bus);
		template <InstructionType type>
		void adm_zero_page(Bus &bus);
		void adm_zero_page_rmw(Bus &bus);
		template <CPURegister reg, InstructionType type>
		void adm_zero_page_indexed(Bus &bus);
		void adm_zero_page_indexed_rmw(Bus &bus);
		template <InstructionType type>
		void adm_absolute(Bus &bus);
		void adm_absolute_rmw(Bus &bus);
		void adm_absolute_jmp(Bus &bus);
		template <CPURegister reg, InstructionType type>
		void adm_absolute_indexed(Bus &bus);
		void adm_absolute_indexed_rmw(Bus &bus);
		template <InstructionType type>
		void adm_indexed_indirect_x(Bus &bus);
		void adm_indexed_indirect_x_rmw(Bus &bus);
		template <InstructionType type>
		void adm_indirect_indexed_y(Bus &bus);
		void adm_indirect_indexed_y_rmw(Bus &bus);

		void decode(uint8_t opcode);
	};
}