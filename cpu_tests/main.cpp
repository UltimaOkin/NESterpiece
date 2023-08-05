#include <nes/core.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <fmt/format.h>

bool test_with_json(std::string path)
{
	using json = nlohmann::json;
	std::ifstream file(path);

	if (file)
	{
		auto jdata = json::parse(file);
		file.close();
		for (const auto &object : jdata)
		{
			bool test_failed = false;
			const auto &initial = object["initial"];
			NESterpiece::Core core{};
			core.cpu.registers.pc = initial["pc"];
			core.cpu.registers.sp = initial["s"];
			core.cpu.registers.a = initial["a"];
			core.cpu.registers.x = initial["x"];
			core.cpu.registers.y = initial["y"];
			core.cpu.registers.sr = initial["p"];
			for (const auto &ram : initial["ram"])
			{
				core.bus.memory[ram[0]] = ram[1];
			}

			size_t num_cycles = object["cycles"].size();
			std::vector<NESterpiece::BusActivity> bus_activity;
			for (auto i = 0; i < num_cycles; ++i)
			{
				core.tick_components(12);
				bus_activity.push_back(core.bus.last_activity);
			}

			const auto &result = object["final"];
			fmt::print("[{}]\n", object["name"].get<std::string>());

			if ((result["pc"] != core.cpu.registers.pc) ||
				(result["s"] != core.cpu.registers.sp) ||
				(result["a"] != core.cpu.registers.a) ||
				(result["x"] != core.cpu.registers.x) ||
				(result["y"] != core.cpu.registers.y) ||
				(result["p"] != core.cpu.registers.sr))
			{
				fmt::print("pc: {:#x} - expected pc: {:#x}\n", core.cpu.registers.pc, result["pc"].get<uint16_t>());
				fmt::print("sp: {:#x} - expected sp: {:#x}\n", core.cpu.registers.sp, result["s"].get<uint8_t>());
				fmt::print(" a: {:#x} - expected  a: {:#x}\n", core.cpu.registers.a, result["a"].get<uint8_t>());
				fmt::print(" x: {:#x} - expected  x: {:#x}\n", core.cpu.registers.x, result["x"].get<uint8_t>());
				fmt::print(" y: {:#x} - expected  y: {:#x}\n", core.cpu.registers.a, result["y"].get<uint8_t>());
				fmt::print(" p: {:#x} - expected  p: {:#x}\n", core.cpu.registers.sr, result["p"].get<uint8_t>());

				test_failed = true;
			}

			for (const auto &ram : result["ram"])
			{
				core.bus.memory[ram[0]] = ram[1];
				if (core.bus.memory[ram[0]] != ram[1])
				{
					fmt::print("\n{:#x}: {:#x} - expected: {:#x}\n", ram[0].get<uint16_t>(), core.bus.memory[ram[0]], ram[1].get<uint8_t>());
					test_failed = true;
				}
			}

			for (auto i = 0; i < bus_activity.size(); ++i)
			{
				const auto &act = bus_activity[i];
				const auto &cycle = object["cycles"][i];
				constexpr std::array<std::string_view, 2> rw{
					"read",
					"write",
				};

				if ((act.address != cycle[0].get<uint16_t>()) ||
					(act.value != cycle[1].get<uint16_t>()) ||
					(rw[act.type] != cycle[2].get<std::string>()))
				{
					fmt::print("\ncycle: {}\n", i + 1);
					fmt::print("bus address: {:#x} - expected: {:#x}\n", act.address, cycle[0].get<uint16_t>());
					fmt::print("value: {:#x} - expected: {:#x}\n", act.value, cycle[1].get<uint8_t>());
					fmt::print("operation: {}, expected: {}\n", rw[act.type], cycle[2].get<std::string>());
					test_failed = true;
				}
			}

			if (test_failed)
				return false;
		}

		return true;
	}

	fmt::print("Cannot find file for test.\n");
	return false;
}

int main()
{
	if (!test_with_json("a5.json"))
		return 1;
	if (!test_with_json("a9.json"))
		return 1;

	return 0;
}