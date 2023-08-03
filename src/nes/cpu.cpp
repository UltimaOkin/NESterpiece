#include "cpu.hpp"
#include "bus.hpp"
#include "constants.hpp"

namespace NESterpiece
{
	void CPU::reset()
	{
		ticks = 0;
		state = ExecutionState{
			.complete = true,
			.current_cycle = 0, // after the fetch/decode step
			.fetched_data = 0,
			.fetched_address = 0,
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
			{
				(this->*state.addressing_function)(bus);
			}
			else
			{
				decode(bus.read(registers.pc));
			}

			ticks = 0;
		}
		ticks++;
	}

	void CPU::decode(uint8_t opcode)
	{
		uint8_t group = opcode & 0b11;
		uint8_t address = (opcode & 0b11100) >> 2;
		uint8_t instruction = (opcode & 0b11100000) >> 5;

		state = ExecutionState{
			.complete = false,
			.current_cycle = 1,
			.fetched_data = 0,
			.fetched_address = 0,
			.addressing_function = &CPU::immediate_addressing,
			.operation_function = &CPU::lda,
		};
	}

	void CPU::immediate_addressing(Bus &bus)
	{
		switch (state.current_cycle)
		{
		case 1:
		{
			state.fetched_data = bus.read(registers.pc + 1);
			registers.pc++;
			state.current_cycle++;
			break;
		}
		case 2:
		{
			(this->*state.operation_function)(bus);
			state.complete = true;
			break;
		}
		}
	}

	void CPU::lda(Bus &bus)
	{
		// todo: set N and Z flags
		registers.a = state.fetched_data;
	}

	void CPU::sta(Bus &bus)
	{
		bus.write(state.fetched_address, registers.a);
	}

	void CPU::stx(Bus &bus)
	{
		bus.write(state.fetched_address, registers.x);
	}

	void CPU::sty(Bus &bus)
	{
		bus.write(state.fetched_address, registers.y);
	}
}