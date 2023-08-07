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

	void CPU::op_lda(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.sr = data == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = data & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
		registers.a = data;
	}

	void CPU::op_ldx(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.sr = data == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = data & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
		registers.x = data;
	}

	void CPU::op_ldy(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.sr = data == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = data & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
		registers.y = data;
	}

	void CPU::op_eor(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.a ^= data;

		registers.sr = registers.a == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = registers.a & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
	}

	void CPU::op_and(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.a &= data;

		registers.sr = registers.a == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = registers.a & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
	}

	void CPU::op_ora(Bus &bus)
	{
		auto data = static_cast<uint8_t>(state.data & 0xFF);
		registers.a |= data;

		registers.sr = registers.a == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = registers.a & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
	}

	void CPU::op_adc(Bus &bus)
	{
		uint16_t data = state.data & 0xFF;
		uint16_t result = registers.a + data + (registers.sr & StatusFlags::Carry ? 1 : 0);

		registers.sr = result > 0xFF ? registers.sr | StatusFlags::Carry : registers.sr & ~StatusFlags::Carry;
		result &= 0xFF;

		registers.sr = result == 0 ? registers.sr | StatusFlags::Zero : registers.sr & ~StatusFlags::Zero;
		registers.sr = result & 0x80 ? registers.sr | StatusFlags::Negative : registers.sr & ~StatusFlags::Negative;
		registers.sr = ~(registers.a ^ data) & (registers.a ^ result) & 0x80 ? registers.sr | StatusFlags::Overflow : registers.sr & ~StatusFlags::Overflow;

		registers.a = static_cast<uint8_t>(result);
	}

	void CPU::op_sbc(Bus &bus)
	{
		state.data = ~state.data;
		op_adc(bus);
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
#pragma region LDA
		case 0xA1:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xA5:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xA9:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xAD:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xB1:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xB5:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xBD:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xB9:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_lda;
			break;
		}
#pragma endregion LDA

#pragma region LDX
		case 0xA2:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ldx;
			break;
		}
		case 0xA6:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ldx;
			break;
		}
		case 0xB6:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ldx;
			break;
		}
		case 0xAE:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ldx;
			break;
		}
		case 0xBE:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ldx;
			break;
		}
#pragma endregion LDX

#pragma region LDY
		case 0xA0:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ldy;
			break;
		}
		case 0xA4:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ldy;
			break;
		}
		case 0xB4:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ldy;
			break;
		}
		case 0xAC:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ldy;
			break;
		}
		case 0xBC:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ldy;
			break;
		}
#pragma endregion LDY

#pragma region EOR
		case 0x49:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x45:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x55:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x4D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x5D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x59:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x41:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
		case 0x51:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_eor;
			break;
		}
#pragma endregion EOR

#pragma region AND
		case 0x29:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x25:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x35:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x2D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x3D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x39:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x21:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
		case 0x31:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_and;
			break;
		}
#pragma endregion AND

#pragma region ORA
		case 0x09:
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x05:
		{
			state.addressing_function = &CPU::adm_zero_page<InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x15:
		{
			state.addressing_function = &CPU::adm_zero_page_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x0D:
		{
			state.addressing_function = &CPU::adm_absolute<InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x1D:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::X, InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x19:
		{
			state.addressing_function = &CPU::adm_absolute_indexed<CPURegister::Y, InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x01:
		{
			state.addressing_function = &CPU::adm_indexed_indirect_x<InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
		case 0x11:
		{
			state.addressing_function = &CPU::adm_indirect_indexed_y<InstructionType::Read>;
			state.operation_function = &CPU::op_ora;
			break;
		}
#pragma endregion ORA

#pragma region ADC
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
#pragma endregion ADC

#pragma region SBC
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
#pragma endregion SBC

		default: // Illegal/Unimplemented Opcodes
		{
			assert(false);
			break;
		}
		}
	}

}