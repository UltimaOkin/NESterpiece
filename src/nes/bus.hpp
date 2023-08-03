#pragma once
#include <cinttypes>
#include <array>
namespace NESterpiece
{
	class Bus
	{
	public:
		std::array<uint8_t, 65535> memory;

		uint8_t read(uint16_t address) const;
		void write(uint16_t address, uint8_t value);
	};
}