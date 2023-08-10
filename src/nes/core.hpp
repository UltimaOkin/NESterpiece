#pragma once
#include "cpu.hpp"
#include "bus.hpp"
#include <cinttypes>
namespace NESterpiece
{
	class Core
	{
		uint8_t cpu_counter = 0;

	public:
		CPU cpu;
		Bus bus;
		void tick_components(uint32_t rate);
	};
}