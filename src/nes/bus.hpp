#pragma once
#include <cinttypes>
#include <array>
namespace NESterpiece
{
	enum BusActivityType
	{
		Read = 0,
		Write = 1
	};
	struct BusActivity
	{
		uint16_t address = 0;
		uint16_t value = 0;
		BusActivityType type = BusActivityType::Read;
	};

	class Bus
	{
	public:
		std::array<uint8_t, 65535> memory{};
		BusActivity last_activity;
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t value);
	};
}