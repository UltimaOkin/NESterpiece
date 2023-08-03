#pragma once
#include "cpu.hpp"
#include "bus.hpp"
#include <cinttypes>
namespace NESterpiece
{
	class Core
	{
	public:
		CPU cpu;
		Bus bus;
		void tick_components();
	};
}