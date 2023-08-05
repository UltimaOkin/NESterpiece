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

	void CPU::decode(uint8_t opcode)
	{
		state = ExecutionState{
			.complete = false,
			.current_cycle = 1,
			.data = 0,
			.address = 0,
			.addressing_function = nullptr,
			.operation_function = nullptr,
		};

		registers.pc++;

		switch (opcode)
		{
		case 0xA9: // LDA #oper
		{
			state.addressing_function = &CPU::adm_immediate;
			state.operation_function = &CPU::op_lda;
			break;
		}
		case 0xA5: // LDA oper
		{
			state.addressing_function = &CPU::adm_zero_page_r;
			state.operation_function = &CPU::op_lda;
			break;
		}

		default: // Illegal/Unimplemented Opcodes
		{
			assert(false);
			break;
		}
		}
	}

	void CPU::adm_immediate(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1: // fetch immediate value and execute
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

	void CPU::adm_zero_page_r(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1: // fetch zero page address
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2: // read from zero page and execute
		{
			state.data = bus.read(state.address);
			(this->*state.operation_function)(bus);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::adm_zero_page_w(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1: // fetch zero page address
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2: // write to zero page and execute
		{
			(this->*state.operation_function)(bus);
			bus.write(state.address, state.data);
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
		case 1: // fetch zero page address
		{
			state.address = bus.read(registers.pc);
			registers.pc++;
			break;
		}
		case 2: // read from zero page
		{
			state.data = bus.read(state.address);
			break;
		}
		case 3: // dummy write?
		{
			bus.write(state.address, state.data);
			break;
		}
		case 4: // execute and write the modified value
		{
			(this->*state.operation_function)(bus);
			bus.write(state.address, state.data);
			state.complete = true;
			break;
		}
		}
		state.current_cycle++;
	}

	void CPU::op_lda(Bus &bus)
	{
		registers.sr = (state.data == 0) ? (registers.sr | StatusFlags::Zero) : (registers.sr & ~StatusFlags::Zero);
		registers.sr = (state.data & 128) ? (registers.sr | StatusFlags::Negative) : (registers.sr & ~StatusFlags::Negative);
		registers.a = static_cast<uint8_t>(state.data & 0xFF);
	}
}