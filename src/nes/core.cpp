#include "core.hpp"

namespace NESterpiece
{
	void Core::tick_components()
	{
		cpu.step(bus);
	}
}