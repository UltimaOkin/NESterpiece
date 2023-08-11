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
		Blank = 16,
		Break = 32,
		Overflow = 64,
		Negative = 128
	};

	enum class TargetValue
	{
		A,
		X,
		Y,
		S,
		M,
		P
	};

	enum class InstructionType
	{
		Read,
		Write
	};

	enum class BitOp
	{
		XOR,
		AND,
		OR
	};

	enum class Condition
	{
		Carry,
		Zero,
		Negative,
		Overflow
	};

	class CPU
	{
		using cpu_function = void (CPU::*)(Bus &);
		struct ExecutionState
		{
			bool complete = false, page_crossed = false;
			bool branch_taken = false;
			uint8_t current_cycle = 0;
			uint16_t data = 0;
			uint16_t address = 0;
			cpu_function addressing_function = nullptr, operation_function = nullptr;
		};

		ExecutionState state{
			.complete = true,
			.page_crossed = false,
			.branch_taken = false,
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
			uint8_t a = 0, x = 0, y = 0, s = 0xFF, p = 0;
		} registers;

		void reset();
		void step(Bus &bus);

		template <TargetValue val>
		void op_ld_v(Bus &bus);
		template <BitOp bit_op>
		void op_bitwise(Bus &bus);
		void op_adc(Bus &bus);
		void op_sbc(Bus &bus);
		template <TargetValue val>
		void op_cmp_v(Bus &bus);
		void op_bit(Bus &bus);
		template <TargetValue val>
		void op_st_v(Bus &bus);
		template <TargetValue val>
		void op_inc_v(Bus &bus);
		template <TargetValue val>
		void op_dec_v(Bus &bus);
		template <StatusFlags flag>
		void op_clear_f(Bus &bus);
		template <StatusFlags flag>
		void op_set_f(Bus &bus);
		template <TargetValue src, TargetValue dst>
		void op_transfer_vv(Bus &bus);
		template <TargetValue val>
		void op_push_v(Bus &bus);
		template <TargetValue val>
		void op_pop_v(Bus &bus);
		template <TargetValue val>
		void op_asl_v(Bus &bus);
		template <TargetValue val>
		void op_lsr_v(Bus &bus);
		template <TargetValue val>
		void op_rol_v(Bus &bus);
		template <TargetValue val>
		void op_ror_v(Bus &bus);
		template <StatusFlags cond, bool set>
		void op_branch_cs(Bus &bus);

		void adm_brk(Bus &bus);
		void adm_rti(Bus &bus);
		void adm_jsr(Bus &bus);
		void adm_rts(Bus &bus);
		void adm_relative(Bus &bus);
		void adm_pha_php(Bus &bus);
		void adm_pla_plp(Bus &bus);
		template <bool is_nop>
		void adm_implied(Bus &bus);
		void adm_immediate(Bus &bus);
		template <InstructionType type>
		void adm_zero_page(Bus &bus);
		void adm_zero_page_rmw(Bus &bus);
		template <TargetValue reg, InstructionType type>
		void adm_zero_page_indexed(Bus &bus);
		void adm_zero_page_indexed_rmw(Bus &bus);
		template <InstructionType type>
		void adm_absolute(Bus &bus);
		void adm_absolute_rmw(Bus &bus);
		void adm_absolute_jmp(Bus &bus);
		void adm_absolute_indirect_jmp(Bus &bus);
		template <TargetValue reg, InstructionType type>
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