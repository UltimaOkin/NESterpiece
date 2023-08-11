#include "cpu.hpp"
#include "bus.hpp"
#include "constants.hpp"
#include <cassert>

namespace NESterpiece
{
	void CPU::reset()
	{
		state = ExecutionState{
			.complete = true,
			.page_crossed = false,
			.branch_taken = false,
			.current_cycle = 0, // after the fetch/decode step
			.data = 0,
			.address = 0,
			.addressing_function = nullptr,
			.operation_function = nullptr,
		};
		registers = Registers();
	}

	void CPU::step(Bus &bus)
	{
		if (!state.complete)
			(this->*state.addressing_function)(bus);
		else
			decode(bus.read(registers.pc));
	}

	template <TargetValue val>
	void CPU::op_ld_v(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.p = data == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = data & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;

		switch (val)
		{
		case TargetValue::A:
			registers.a = data;
			break;
		case TargetValue::X:
			registers.x = data;
			break;
		case TargetValue::Y:
			registers.y = data;
			break;
		default:
			return;
		}
	}

	template <BitOp bit_op>
	void CPU::op_bitwise(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);

		switch (bit_op)
		{
		case BitOp::XOR:
			registers.a ^= data;
			break;
		case BitOp::AND:
			registers.a &= data;
			break;
		case BitOp::OR:
			registers.a |= data;
			break;
		}

		registers.p = registers.a == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = registers.a & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	void CPU::op_adc(Bus &bus)
	{
		uint16_t data = state.data & 0xFF;
		uint16_t result = registers.a + data + (registers.p & StatusFlags::Carry ? 1 : 0);

		registers.p = result > 0xFF ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
		result &= 0xFF;

		registers.p = result == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = result & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
		registers.p = ~(registers.a ^ data) & (registers.a ^ result) & 0x80 ? registers.p | StatusFlags::Overflow : registers.p & ~StatusFlags::Overflow;

		registers.a = static_cast<uint8_t>(result);
	}

	void CPU::op_sbc(Bus &bus)
	{
		state.data = ~state.data;
		op_adc(bus);
	}

	template <TargetValue val>
	void CPU::op_cmp_v(Bus &bus)
	{
		uint8_t reg = 0;
		switch (val)
		{
		case TargetValue::A:
			reg = registers.a;
			break;
		case TargetValue::X:
			reg = registers.x;
			break;
		case TargetValue::Y:
			reg = registers.y;
			break;
		default:
			return;
		}

		auto data = static_cast<uint8_t>(state.data & 0xFF);
		uint8_t result = (reg - data) & StatusFlags::Negative;
		registers.p &= ~(StatusFlags::Negative | StatusFlags::Zero | StatusFlags::Carry);

		if (reg < data)
			registers.p |= result;
		else if (reg == data)
			registers.p |= StatusFlags::Zero | StatusFlags::Carry;
		else if (reg > data)
			registers.p |= result | StatusFlags::Carry;
	}

	void CPU::op_bit(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		uint8_t result = registers.a & data;
		registers.p = result == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;

		registers.p &= ~(StatusFlags::Negative | StatusFlags::Overflow);
		registers.p |= data & 0xC0;
	}

	template <TargetValue val>
	void CPU::op_st_v(Bus &bus)
	{
		uint8_t reg = 0;
		switch (val)
		{
		case TargetValue::A:
			reg = registers.a;
			break;
		case TargetValue::X:
			reg = registers.x;
			break;
		case TargetValue::Y:
			reg = registers.y;
			break;
		default:
			return;
		}
		state.data = reg;
	}

	template <TargetValue val>
	void CPU::op_inc_v(Bus &bus)
	{
		uint8_t result = 0;
		switch (val)
		{
		case TargetValue::A:
			result = ++registers.a;
			break;
		case TargetValue::X:
			result = ++registers.x;
			break;
		case TargetValue::Y:
			result = ++registers.y;
			break;
		case TargetValue::S:
			result = ++registers.s;
			break;
		case TargetValue::M:
			++state.data;
			state.data &= 0xFF;
			result = static_cast<uint8_t>(state.data);
			break;
		default:
			return;
		}

		registers.p = result == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = result & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	template <TargetValue val>
	void CPU::op_dec_v(Bus &bus)
	{
		uint8_t result = 0;
		switch (val)
		{
		case TargetValue::A:
			result = --registers.a;
			break;
		case TargetValue::X:
			result = --registers.x;
			break;
		case TargetValue::Y:
			result = --registers.y;
			break;
		case TargetValue::S:
			result = --registers.s;
			break;
		case TargetValue::M:
			--state.data;
			state.data &= 0xFF;
			result = static_cast<uint8_t>(state.data);
			break;
		default:
			return;
		}

		registers.p = result == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = result & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	template <StatusFlags flag>
	void CPU::op_clear_f(Bus &bus)
	{
		registers.p &= ~flag;
	}

	template <StatusFlags flag>
	void CPU::op_set_f(Bus &bus)
	{
		registers.p |= flag;
	}

	template <TargetValue src, TargetValue dst>
	void CPU::op_transfer_vv(Bus &bus)
	{
		uint8_t *src_value = nullptr, *dst_value = nullptr;
		switch (src)
		{
		case TargetValue::A:
			src_value = &registers.a;
			break;
		case TargetValue::X:
			src_value = &registers.x;
			break;
		case TargetValue::Y:
			src_value = &registers.y;
			break;
		case TargetValue::S:
			src_value = &registers.s;
			break;
		default:
			return;
		}

		switch (dst)
		{
		case TargetValue::A:
			dst_value = &registers.a;
			break;
		case TargetValue::X:
			dst_value = &registers.x;
			break;
		case TargetValue::Y:
			dst_value = &registers.y;
			break;
		case TargetValue::S:
			dst_value = &registers.s;
			break;
		default:
			return;
		}

		*dst_value = *src_value;

		if constexpr (dst != TargetValue::S)
		{
			registers.p = *dst_value == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
			registers.p = *dst_value & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
		}
	}

	template <TargetValue val>
	void CPU::op_push_v(Bus &bus)
	{
		switch (val)
		{
		case TargetValue::A:
			state.data = registers.a;
			break;

		case TargetValue::P:
			state.data = registers.p | StatusFlags::Break | StatusFlags::Blank;
			break;
		default:
			return;
		}
	}

	template <TargetValue val>
	void CPU::op_pop_v(Bus &bus)
	{
		switch (val)
		{
		case TargetValue::A:
		{
			registers.a = state.data;
			registers.p = registers.a == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
			registers.p = registers.a & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
			break;
		}
		case TargetValue::P:
		{
			registers.p = (state.data | StatusFlags::Break) & ~StatusFlags::Blank;
			break;
		}
		default:
		{
			return;
		}
		}
	}

	template <TargetValue val>
	void CPU::op_asl_v(Bus &bus)
	{
		uint8_t out = 0;
		switch (val)
		{
		case TargetValue::A:
		{
			registers.p = registers.a & 0x80 ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
			registers.a <<= 1;
			out = registers.a;
			break;
		}
		case TargetValue::M:
		{
			registers.p = state.data & 0x80 ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
			state.data <<= 1;
			state.data &= 0xFF;
			out = static_cast<uint8_t>(state.data);
			break;
		}
		default:
		{
			return;
		}
		}

		registers.p = out == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = out & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	template <TargetValue val>
	void CPU::op_lsr_v(Bus &bus)
	{
		uint8_t out = 0;
		switch (val)
		{
		case TargetValue::A:
		{
			registers.p = registers.a & 1 ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
			registers.a >>= 1;
			out = registers.a;
			break;
		}
		case TargetValue::M:
		{
			registers.p = state.data & 1 ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
			state.data >>= 1;
			state.data &= 0xFF;
			out = static_cast<uint8_t>(state.data);
			break;
		}
		default:
		{
			return;
		}
		}

		registers.p = out == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p &= ~StatusFlags::Negative;
	}

	template <TargetValue val>
	void CPU::op_rol_v(Bus &bus)
	{
		uint8_t out = 0, out_carry = 0;

		switch (val)
		{
		case TargetValue::A:
		{
			out_carry = registers.a & 0x80;
			registers.a <<= 1;
			registers.a |= registers.p & StatusFlags::Carry ? 1 : 0;
			out = registers.a;
			break;
		}
		case TargetValue::M:
		{
			out_carry = state.data & 0x80;
			state.data <<= 1;
			state.data |= registers.p & StatusFlags::Carry ? 1 : 0;
			state.data &= 0xFF;
			out = static_cast<uint8_t>(state.data);
			break;
		}
		default:
		{
			return;
		}
		}

		registers.p = out_carry ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
		registers.p = out == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = out & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	template <TargetValue val>
	void CPU::op_ror_v(Bus &bus)
	{
		uint8_t out = 0, out_carry = 0;

		switch (val)
		{
		case TargetValue::A:
		{
			out_carry = registers.a & 1;
			registers.a >>= 1;
			registers.a |= registers.p & StatusFlags::Carry ? 0x80 : 0;
			out = registers.a;
			break;
		}
		case TargetValue::M:
		{
			out_carry = state.data & 1;
			state.data >>= 1;
			state.data |= registers.p & StatusFlags::Carry ? 0x80 : 0;
			state.data &= 0xFF;
			out = static_cast<uint8_t>(state.data);
			break;
		}
		default:
		{
			return;
		}
		}

		registers.p = out_carry ? registers.p | StatusFlags::Carry : registers.p & ~StatusFlags::Carry;
		registers.p = out == 0 ? registers.p | StatusFlags::Zero : registers.p & ~StatusFlags::Zero;
		registers.p = out & StatusFlags::Negative ? registers.p | StatusFlags::Negative : registers.p & ~StatusFlags::Negative;
	}

	template <StatusFlags cond, bool set>
	void CPU::op_branch_cs(Bus &bus)
	{
		state.branch_taken = set ? (registers.p & cond) : !(registers.p & cond);
	}

	void CPU::adm_brk(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, static_cast<uint8_t>(registers.pc >> 8));
			registers.s--;
			break;
		}
		case 3:
		{
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, static_cast<uint8_t>(registers.pc & 0xFF));
			registers.s--;
			break;
		}
		case 4:
		{
			// determine interrupt vector
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, registers.p | StatusFlags::Break | StatusFlags::Blank);
			registers.p |= StatusFlags::IRQ;
			registers.s--;
			break;
		}
		case 5:
		{
			state.address = bus.read(0xFFFE);
			break;
		}
		case 6:
		{
			state.address |= static_cast<uint16_t>(bus.read(0xFFFF)) << 8;
			registers.pc = state.address;
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_rti(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.data = bus.read(registers.pc);
			break;
		}
		case 2:
		{
			bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.s++;
			break;
		}
		case 3:
		{
			registers.p = (bus.read(static_cast<uint16_t>(registers.s) | 0x100) | StatusFlags::Break) & ~StatusFlags::Blank;
			registers.s++;
			break;
		}
		case 4:
		{
			state.address = bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.s++;
			break;
		}
		case 5:
		{
			state.address |= static_cast<uint16_t>(bus.read(static_cast<uint16_t>(registers.s) | 0x100)) << 8;
			registers.pc = state.address;
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_jsr(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.data = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			break;
		}

		case 3:
		{
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, static_cast<uint8_t>(registers.pc >> 8));
			registers.s--;
			break;
		}
		case 4:
		{
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, static_cast<uint8_t>(registers.pc & 0xFF));
			registers.s--;
			break;
		}
		case 5:
		{
			uint16_t pcl = state.data;
			uint16_t pch = bus.read(registers.pc);

			registers.pc = (pch << 8) | pcl;
			state.complete = true;
			break;
		}
		}

		state.current_cycle++;
	}

	void CPU::adm_rts(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
			break;
		}
		case 2:
		{
			bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.s++;
			break;
		}
		case 3:
		{
			state.data = bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.s++;
			break;
		}
		case 4:
		{
			uint16_t pch = bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.pc = (pch << 8) | state.data;
			break;
		}
		case 5:
		{
			bus.read(registers.pc);
			registers.pc++;
			break;
		}
		}

		state.current_cycle++;
	}

	void CPU::adm_relative(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.data = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			bus.read(registers.pc);
			(this->*state.operation_function)(bus);
			if (state.branch_taken)
			{
				uint16_t pcl = registers.pc & 0xFF;
				int8_t offset = static_cast<int8_t>(state.data & 0xFF);
				pcl += offset;

				if (pcl > 0xFF)
					state.page_crossed = true;

				registers.pc &= 0xFF00;
				registers.pc |= pcl & 0xFF;
			}
			else
			{
				state.complete = true;
			}
			break;
		}
		case 3:
		{
			bus.read(registers.pc);

			if (state.page_crossed)
			{
				uint8_t pch = static_cast<uint8_t>(registers.pc >> 8);

				pch += state.data & 0x80 ? -1 : 1;
				registers.pc &= 0xFF;
				registers.pc |= static_cast<uint16_t>(pch) << 8;
			}
			else
			{
				state.complete = true;
			}

			break;
		}
		case 4:
		{
			bus.read(registers.pc);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_pha_php(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
			break;
		}
		case 2:
		{
			(this->*state.operation_function)(bus);
			bus.write(static_cast<uint16_t>(registers.s) | 0x100, state.data);
			registers.s--;
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_pla_plp(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
			break;
		}
		case 2:
		{

			bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			registers.s++;
			break;
		}
		case 3:
		{
			state.data = bus.read(static_cast<uint16_t>(registers.s) | 0x100);
			(this->*state.operation_function)(bus);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <bool is_nop>
	void CPU::adm_implied(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
			if constexpr (!is_nop)
				(this->*state.operation_function)(bus);

			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_immediate(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.data = bus.read(registers.pc);
			(this->*state.operation_function)(bus);
			state.complete = true;
			registers.pc++;
			break;
		}
		}
		state.current_cycle++;
	}

	template <InstructionType type>
	void CPU::adm_zero_page(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}

			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_zero_page_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 3:
		{
			bus.write(state.address, state.data);
			(this->*state.operation_function)(bus);
			break;
		}
		case 4:
		{
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <TargetValue reg, InstructionType type>
	void CPU::adm_zero_page_indexed(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);

			if constexpr (reg == TargetValue::X)
				state.address += registers.x;
			else if constexpr (reg == TargetValue::Y)
				state.address += registers.y;

			state.address &= 0xFF;
			break;
		}
		case 3:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_zero_page_indexed_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			state.address += registers.x;
			state.address &= 0xFF;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 4:
		{
			bus.write(state.address, state.data);
			(this->*state.operation_function)(bus);
			break;
		}
		case 5:
		{
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <InstructionType type>
	void CPU::adm_absolute(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc++;
			break;
		}
		case 3:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_absolute_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc++;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 4:
		{
			bus.write(state.address, state.data);
			(this->*state.operation_function)(bus);
			break;
		}
		case 5:
		{
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_absolute_jmp(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc = state.address;
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_absolute_indirect_jmp(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc++;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 4:
		{
			auto adl = state.address & 0xFF;
			adl++;
			adl &= 0xFF;
			state.address &= 0xFF00;
			state.address |= adl;
			state.data |= static_cast<uint16_t>(bus.read(state.address)) << 8;
			registers.pc = state.data;
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <TargetValue reg, InstructionType type>
	void CPU::adm_absolute_indexed(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			uint16_t low = state.address;

			if constexpr (reg == TargetValue::X)
			{
				state.address += registers.x;

				if ((low + registers.x) > 0xFF)
					state.page_crossed = true;
			}
			else if constexpr (reg == TargetValue::Y)
			{
				state.address += registers.y;
				if ((low + registers.y) > 0xFF)
					state.page_crossed = true;
			}
			state.address &= 0xFF;
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc++;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			if (state.page_crossed)
			{
				uint16_t high = (state.address >> 8) & 0xFF;
				high++;
				state.address &= 0xFF;
				state.address |= high << 8;
			}
			else
			{
				if constexpr (type == InstructionType::Read)
				{
					(this->*state.operation_function)(bus);
					state.complete = true;
				}
			}
			break;
		}
		case 4:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_absolute_indexed_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			uint16_t low = state.address;
			state.address += registers.x;

			if ((low + registers.x) > 0xFF)
				state.page_crossed = true;

			state.address &= 0xFF;
			state.address |= static_cast<uint16_t>(bus.read(registers.pc)) << 8;
			registers.pc++;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			if (state.page_crossed)
			{
				uint16_t high = (state.address >> 8) & 0xFF;
				high++;
				state.address &= 0xFF;
				state.address |= high << 8;
			}

			break;
		}
		case 4:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 5:
		{
			bus.write(state.address, state.data);
			break;
		}
		case 6:
		{
			(this->*state.operation_function)(bus);
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <InstructionType type>
	void CPU::adm_indexed_indirect_x(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			state.address += registers.x;
			state.address &= 0xFF;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 4:
		{
			state.data |= static_cast<uint16_t>(bus.read((state.address + 1) & 0xFF)) << 8;
			state.address = state.data;
			break;
		}
		case 5:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_indexed_indirect_x_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			state.address += registers.x;
			state.address &= 0xFF;
			break;
		}
		case 3:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 4:
		{
			state.data |= static_cast<uint16_t>(bus.read((state.address + 1) & 0xFF)) << 8;
			state.address = state.data;
			break;
		}
		case 5:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 6:
		{
			bus.write(state.address, state.data);
			(this->*state.operation_function)(bus);
			break;
		}
		case 7:
		{
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	template <InstructionType type>
	void CPU::adm_indirect_indexed_y(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 3:
		{
			uint16_t low = state.data;
			state.data = 0;
			state.data |= static_cast<uint16_t>(bus.read((state.address + 1) & 0xFF)) << 8;
			low += registers.y;
			if (low > 0xFF)
			{
				low &= 0xFF;
				state.page_crossed = true;
			}

			state.address = state.data | low;

			break;
		}
		case 4:
		{
			state.data = bus.read(state.address);

			if (state.page_crossed)
			{
				uint16_t high = (state.address >> 8) & 0xFF;
				high++;
				state.address &= 0xFF;
				state.address |= high << 8;
			}
			else
			{
				if constexpr (type == InstructionType::Read)
				{
					(this->*state.operation_function)(bus);
					state.complete = true;
				}
			}

			break;
		}
		case 5:
		{
			if constexpr (type == InstructionType::Read)
			{
				state.data = bus.read(state.address);
				(this->*state.operation_function)(bus);
			}
			else if constexpr (type == InstructionType::Write)
			{
				(this->*state.operation_function)(bus);
				bus.write(state.address, state.data);
			}
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_indirect_indexed_y_rmw(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 3:
		{
			uint16_t low = state.data;
			state.data = 0;
			state.data |= static_cast<uint16_t>(bus.read((state.address + 1) & 0xFF)) << 8;
			low += registers.y;
			if (low > 0xFF)
			{
				low &= 0xFF;
				state.page_crossed = true;
			}

			state.address = state.data | low;

			break;
		}
		case 4:
		{
			state.data = bus.read(state.address);

			if (state.page_crossed)
			{
				uint16_t high = (state.address >> 8) & 0xFF;
				high++;
				state.address &= 0xFF;
				state.address |= high << 8;
			}

			break;
		}
		case 5:
		{
			state.data = bus.read(state.address);
			break;
		}
		case 6:
		{
			bus.write(state.address, state.data);
			(this->*state.operation_function)(bus);
			break;
		}
		case 7:
		{
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::decode(uint8_t opcode)
	{
		state = ExecutionState{
			.complete = false,
			.page_crossed = false,
			.branch_taken = false,
			.current_cycle = 1,
			.data = 0,
			.address = 0,
			.addressing_function = nullptr,
			.operation_function = nullptr,
		};

		registers.pc++;

		switch (opcode)
		{
		// LDA
		case 0xA1:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xA5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xA9:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xAD:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xB1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xB5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xBD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}
		case 0xB9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::A>;
			break;
		}

		// LDX
		case 0xA2:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_v<TargetValue::X>;
			break;
		}
		case 0xA6:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::X>;
			break;
		}
		case 0xB6:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::X>;
			break;
		}
		case 0xAE:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::X>;
			break;
		}
		case 0xBE:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::X>;
			break;
		}

		// LDY
		case 0xA0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_v<TargetValue::Y>;
			break;
		}
		case 0xA4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::Y>;
			break;
		}
		case 0xB4:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::Y>;
			break;
		}
		case 0xAC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::Y>;
			break;
		}
		case 0xBC:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_v<TargetValue::Y>;
			break;
		}

		// EOR
		case 0x49:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x45:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x55:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x4D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x5D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x59:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x41:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x51:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}

		// AND
		case 0x29:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x25:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x35:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x2D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x3D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x39:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x21:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x31:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}

		// ORA
		case 0x09:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x05:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x15:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x0D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x1D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x19:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x01:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x11:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}

		// ADC
		case 0x69:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x65:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x75:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x6D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x7D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x79:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x61:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x71:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}

		// SBC
		case 0xE9:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xE5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xF5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xED:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xFD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xF9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xE1:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xF1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}

		// CMP
		case 0xC9:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xC5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xD5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xCD:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xDD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xD9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xC1:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}
		case 0xD1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::A>;
			break;
		}

		// CPX
		case 0xE0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_cmp_v<TargetValue::X>;
			break;
		}
		case 0xE4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::X>;
			break;
		}
		case 0xEC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::X>;
			break;
		}

		// CPY
		case 0xC0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_cmp_v<TargetValue::Y>;
			break;
		}
		case 0xC4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::Y>;
			break;
		}
		case 0xCC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_v<TargetValue::Y>;
			break;
		}

		// BIT
		case 0x24:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_bit;
			break;
		}
		case 0x2C:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_bit;
			break;
		}

		// STA
		case 0x85:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x95:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x8D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x9D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x99:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<TargetValue::Y, InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x81:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}
		case 0x91:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::A>;
			break;
		}

		// STX
		case 0x86:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::X>;
			break;
		}
		case 0x96:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::Y, InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::X>;
			break;
		}
		case 0x8E:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::X>;
			break;
		}

		// STY
		case 0x84:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::Y>;
			break;
		}
		case 0x94:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<TargetValue::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::Y>;
			break;
		}
		case 0x8C:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_v<TargetValue::Y>;
			break;
		}

		// INC
		case 0xE6:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_inc_v<TargetValue::M>;
			break;
		}
		case 0xF6:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_inc_v<TargetValue::M>;
			break;
		}
		case 0xEE:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_inc_v<TargetValue::M>;
			break;
		}
		case 0xFE:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_inc_v<TargetValue::M>;
			break;
		}

		// INX
		case 0xE8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_inc_v<TargetValue::X>;
			break;
		}

		// INY
		case 0xC8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_inc_v<TargetValue::Y>;
			break;
		}

		// DEC
		case 0xC6:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_dec_v<TargetValue::M>;
			break;
		}
		case 0xD6:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_dec_v<TargetValue::M>;
			break;
		}
		case 0xCE:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_dec_v<TargetValue::M>;
			break;
		}
		case 0xDE:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_dec_v<TargetValue::M>;
			break;
		}

		// DEX
		case 0xCA:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_dec_v<TargetValue::X>;
			break;
		}

		// DEY
		case 0x88:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_dec_v<TargetValue::Y>;
			break;
		}

		// CLC
		case 0x18:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_clear_f<StatusFlags::Carry>;
			break;
		}

		// CLD
		case 0xD8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_clear_f<StatusFlags::Decimal>;
			break;
		}

		// CLI
		case 0x58:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_clear_f<StatusFlags::IRQ>;
			break;
		}

		// CLV
		case 0xB8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_clear_f<StatusFlags::Overflow>;
			break;
		}

		// SEC
		case 0x38:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_set_f<StatusFlags::Carry>;
			break;
		}

		// SED
		case 0xF8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_set_f<StatusFlags::Decimal>;
			break;
		}

		// SEI
		case 0x78:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_set_f<StatusFlags::IRQ>;
			break;
		}

		// TAX
		case 0xAA:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::A, TargetValue::X>;
			break;
		}

		// TAY
		case 0xA8:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::A, TargetValue::Y>;
			break;
		}

		// TSX
		case 0xBA:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::S, TargetValue::X>;
			break;
		}

		// TXA
		case 0x8A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::X, TargetValue::A>;
			break;
		}

		// TXS
		case 0x9A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::X, TargetValue::S>;
			break;
		}

		// TYA
		case 0x98:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_transfer_vv<TargetValue::Y, TargetValue::A>;
			break;
		}

		// PHA
		case 0x48:
		{
			state.addressing_function = &CPU::adm_pha_php;
			state.operation_function = &CPU::op_push_v<TargetValue::A>;
			break;
		}

		// PHP
		case 0x08:
		{
			state.addressing_function = &CPU::adm_pha_php;
			state.operation_function = &CPU::op_push_v<TargetValue::P>;
			break;
		}

		// PLA
		case 0x68:
		{
			state.addressing_function = &CPU::adm_pla_plp;
			state.operation_function = &CPU::op_pop_v<TargetValue::A>;
			break;
		}

		// PLP
		case 0x28:
		{
			state.addressing_function = &CPU::adm_pla_plp;
			state.operation_function = &CPU::op_pop_v<TargetValue::P>;
			break;
		}

		// ASL
		case 0x0A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_asl_v<TargetValue::A>;
			break;
		}
		case 0x06:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_asl_v<TargetValue::M>;
			break;
		}
		case 0x16:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_asl_v<TargetValue::M>;
			break;
		}
		case 0x0E:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_asl_v<TargetValue::M>;
			break;
		}
		case 0x1E:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_asl_v<TargetValue::M>;
			break;
		}

		// LSR
		case 0x4A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_lsr_v<TargetValue::A>;
			break;
		}
		case 0x46:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_lsr_v<TargetValue::M>;
			break;
		}
		case 0x56:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_lsr_v<TargetValue::M>;
			break;
		}
		case 0x4E:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_lsr_v<TargetValue::M>;
			break;
		}
		case 0x5E:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_lsr_v<TargetValue::M>;
			break;
		}

		// ROL
		case 0x2A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_rol_v<TargetValue::A>;
			break;
		}
		case 0x26:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_rol_v<TargetValue::M>;
			break;
		}
		case 0x36:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_rol_v<TargetValue::M>;
			break;
		}
		case 0x2E:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_rol_v<TargetValue::M>;
			break;
		}
		case 0x3E:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_rol_v<TargetValue::M>;
			break;
		}

		// ROR
		case 0x6A:
		{
			state.addressing_function = &CPU::adm_implied<false>;
			state.operation_function = &CPU::op_ror_v<TargetValue::A>;
			break;
		}
		case 0x66:
		{
			state.addressing_function = &CPU::adm_zero_page_rmw;
			state.operation_function = &CPU::op_ror_v<TargetValue::M>;
			break;
		}
		case 0x76:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed_rmw;
			state.operation_function = &CPU::op_ror_v<TargetValue::M>;
			break;
		}
		case 0x6E:
		{
			state.addressing_function = &CPU::adm_absolute_rmw;
			state.operation_function = &CPU::op_ror_v<TargetValue::M>;
			break;
		}
		case 0x7E:
		{
			state.addressing_function = &CPU::adm_absolute_indexed_rmw;
			state.operation_function = &CPU::op_ror_v<TargetValue::M>;
			break;
		}

		// BCC
		case 0x90:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Carry, false>;
			break;
		}

		// BCS
		case 0xB0:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Carry, true>;
			break;
		}

		// BEQ
		case 0xF0:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Zero, true>;
			break;
		}

		// BMI
		case 0x30:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Negative, true>;
			break;
		}

		// BNE
		case 0xD0:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Zero, false>;
			break;
		}

		// BPL
		case 0x10:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Negative, false>;
			break;
		}

		// BVC
		case 0x50:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Overflow, false>;
			break;
		}

		// BVS
		case 0x70:
		{
			state.addressing_function = &CPU::adm_relative;
			state.operation_function = &CPU::op_branch_cs<StatusFlags::Overflow, true>;
			break;
		}

		// BRK
		case 0x00:
		{
			state.addressing_function = &CPU::adm_brk;
			break;
		}

		// RTI
		case 0x40:
		{
			state.addressing_function = &CPU::adm_rti;
			break;
		}

		// JSR
		case 0x20:
		{
			state.addressing_function = &CPU::adm_jsr;
			break;
		}

		// RTS
		case 0x60:
		{
			state.addressing_function = &CPU::adm_rts;
			break;
		}

		// JMP
		case 0x4C:
		{
			state.addressing_function = &CPU::adm_absolute_jmp;
			break;
		}
		case 0x6C:
		{
			state.addressing_function = &CPU::adm_absolute_indirect_jmp;
			break;
		}

		// NOP
		case 0xEA:
		{
			state.addressing_function = &CPU::adm_implied<true>;
			break;
		}

		default: // Illegal/Unimplemented Opcodes
		{
			assert(false);
			break;
		}
		}
	}
}