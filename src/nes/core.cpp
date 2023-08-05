#include "core.hpp"

namespace NESterpiece
{
	void Core::tick_components(int32_t rate)
	{
		do
		{
			cpu.step(bus);
		} while (rate-- > 0);
	}
}