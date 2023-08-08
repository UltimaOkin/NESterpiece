#include "cpu.hpp"
#include "bus.hpp"
#include "constants.hpp"
#include <cassert>

namespace NESterpiece
{
	void CPU::reset()
	{
		ticks = 0;
		state = ExecutionState{
			.complete = true,
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
		if (ticks == CPU_CLOCK_RATE)
		{
			if (!state.complete)
				(this->*state.addressing_function)(bus);
			else
				decode(bus.read(registers.pc));

			ticks = 0;
		}
		ticks++;
	}

	template <CPURegister cpu_reg>
	void CPU::op_ld_r(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.sr = data == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = data & StatusFlags::Negative ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;

		switch (cpu_reg)
		{
		case CPURegister::A:
			registers.a = data;
			break;
		case CPURegister::X:
			registers.x = data;
			break;
		case CPURegister::Y:
			registers.y = data;
			break;
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

		registers.sr = registers.a == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = registers.a & StatusFlags::Negative ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
	}

	void CPU::op_adc(Bus &bus)
	{
		uint16_t data = state.data & 0xFF;
		uint16_t result = registers.a + data + (registers.sr & StatusFlags::Carry ? 1 : 0);

		registers.sr = result > 0xFF ? registers.sr | StatusFlags::Carry : registers.sr & ~StatusFlags::Carry;
		result &= 0xFF;

		registers.sr = result == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = result & StatusFlags::Negative ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
		registers.sr = ~(registers.a ^ data) & (registers.a ^ result) & 0x80 ? registers.sr | StatusFlags::Overflow : registers.sr & ~StatusFlags::Overflow;

		registers.a = static_cast<uint8_t>(result);
	}

	void CPU::op_sbc(Bus &bus)
	{
		state.data = ~state.data;
		op_adc(bus);
	}

	template <CPURegister cpu_reg>
	void CPU::op_cmp_r(Bus &bus)
	{
		uint8_t reg = 0;
		switch (cpu_reg)
		{
		case CPURegister::A:
			reg = registers.a;
			break;
		case CPURegister::X:
			reg = registers.x;
			break;
		case CPURegister::Y:
			reg = registers.y;
			break;
		}

		auto data = static_cast<uint8_t>(state.data & 0xFF);
		uint8_t result = (reg - data) & StatusFlags::Negative;
		registers.sr &= ~(StatusFlags::Negative | StatusFlags::Zero | StatusFlags::Carry);

		if (reg < data)
			registers.sr |= result;
		else if (reg == data)
			registers.sr |= StatusFlags::Zero | StatusFlags::Carry;
		else if (reg > data)
			registers.sr |= result | StatusFlags::Carry;
	}

	void CPU::op_bit(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		uint8_t result = registers.a & data;
		registers.sr = result == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;

		registers.sr &= ~(StatusFlags::Negative | StatusFlags::Overflow);
		registers.sr |= data & 0xC0;
	}

	template <CPURegister cpu_reg>
	void CPU::op_st_r(Bus &bus)
	{
		uint8_t reg = 0;
		switch (cpu_reg)
		{
		case CPURegister::A:
			reg = registers.a;
			break;
		case CPURegister::X:
			reg = registers.x;
			break;
		case CPURegister::Y:
			reg = registers.y;
			break;
		}
		state.data = reg;
	}

	void CPU::adm_implied(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			bus.read(registers.pc);
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

	template <CPURegister reg, InstructionType type>
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

			if constexpr (reg == CPURegister::X)
				state.address += registers.x;
			else if constexpr (reg == CPURegister::Y)
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

	template <CPURegister reg, InstructionType type>
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

			if constexpr (reg == CPURegister::X)
			{
				state.address += registers.x;

				if ((low + registers.x) > 0xFF)
					state.page_crossed = true;
			}
			else if constexpr (reg == CPURegister::Y)
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
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xA5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xA9:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xAD:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xB1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xB5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xBD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}
		case 0xB9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::A>;
			break;
		}

		// LDX
		case 0xA2:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_r<CPURegister::X>;
			break;
		}
		case 0xA6:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::X>;
			break;
		}
		case 0xB6:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::X>;
			break;
		}
		case 0xAE:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::X>;
			break;
		}
		case 0xBE:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::X>;
			break;
		}

		// LDY
		case 0xA0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ld_r<CPURegister::Y>;
			break;
		}
		case 0xA4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::Y>;
			break;
		}
		case 0xB4:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::Y>;
			break;
		}
		case 0xAC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::Y>;
			break;
		}
		case 0xBC:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ld_r<CPURegister::Y>;
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
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::XOR>;
			break;
		}
		case 0x59:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::AND>;
			break;
		}
		case 0x39:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_bitwise<BitOp::OR>;
			break;
		}
		case 0x19:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_adc;
			break;
		}
		case 0x79:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
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
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_sbc;
			break;
		}
		case 0xF9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
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
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xC5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xD5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xCD:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xDD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xD9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xC1:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}
		case 0xD1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::A>;
			break;
		}

		// CPX
		case 0xE0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_cmp_r<CPURegister::X>;
			break;
		}
		case 0xE4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::X>;
			break;
		}
		case 0xEC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::X>;
			break;
		}

		// CPY
		case 0xC0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_cmp_r<CPURegister::Y>;
			break;
		}
		case 0xC4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::Y>;
			break;
		}
		case 0xCC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_cmp_r<CPURegister::Y>;
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
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x95:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x8D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x9D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x99:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x81:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}
		case 0x91:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::A>;
			break;
		}

		// STX
		case 0x86:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::X>;
			break;
		}
		case 0x96:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::Y, InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::X>;
			break;
		}
		case 0x8E:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::X>;
			break;
		}

		// STY
		case 0x84:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::Y>;
			break;
		}
		case 0x94:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::Y>;
			break;
		}
		case 0x8C:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Write>;
			state.operation_function = &CPU::op_st_r<CPURegister::Y>;
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