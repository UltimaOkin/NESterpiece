#include "bus.hpp"
#include "../src/nes/cpu.hpp"
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
		NESterpiece::CPU cpu{};
		NESterpiece::Bus bus{};
		for (const auto &object : jdata)
		{
			bool test_failed = false;
			const auto &initial = object["initial"];

			cpu.reset_to_address(0);
			bus.memory.fill(0);
			bus.activity_list.clear();
			cpu.registers.pc = initial["pc"];
			cpu.registers.s = initial["s"];
			cpu.registers.a = initial["a"];
			cpu.registers.x = initial["x"];
			cpu.registers.y = initial["y"];
			cpu.registers.p = initial["p"];

			for (const auto &ram : initial["ram"])
			{
				bus.memory[ram[0]] = ram[1];
			}

			size_t num_cycles = object["cycles"].size();

			if (object["name"] == "90 91 aa")
			{
				int d = 0;
			}

			cpu.step(bus);
			const auto &result = object["final"];

			if (result["pc"] != cpu.registers.pc)
			{
				fmt::print("pc: {} - expected pc: {}\n", cpu.registers.pc, result["pc"].get<uint16_t>());
				test_failed = true;
			}

			if (result["s"] != cpu.registers.s)
			{
				fmt::print("sp: {:#x} - expected sp: {:#x}\n", cpu.registers.s, result["s"].get<uint8_t>());
				test_failed = true;
			}

			if (result["a"] != cpu.registers.a)
			{
				fmt::print(" a: {:#x} - expected  a: {:#x}\n", cpu.registers.a, result["a"].get<uint8_t>());
				test_failed = true;
			}

			if (result["x"] != cpu.registers.x)
			{
				fmt::print(" x: {:#x} - expected  x: {:#x}\n", cpu.registers.x, result["x"].get<uint8_t>());
				test_failed = true;
			}

			if (result["y"] != cpu.registers.y)
			{
				fmt::print(" y: {:#x} - expected  y: {:#x}\n", cpu.registers.y, result["y"].get<uint8_t>());
				test_failed = true;
			}
			if (result["p"] != cpu.registers.p)
			{
				fmt::print(" p: {:#x} - expected  p: {:#x}\n", cpu.registers.p, result["p"].get<uint8_t>());
				test_failed = true;
			}

			for (const auto &ram : result["ram"])
			{
				if (bus.memory[ram[0]] != ram[1])
				{
					fmt::print("\n{:#x}: {:#x} - expected: {:#x}\n", ram[0].get<uint16_t>(), bus.memory[ram[0]], ram[1].get<uint8_t>());
					test_failed = true;
				}
			}
			bool bad_cycles = false;
			if (bus.activity_list.size() != object["cycles"].size())
			{
				fmt::print("Incorrect number of cycles taken: {} - expected: {}\n", bus.activity_list.size(), object["cycles"].size());

				for (int i = 0; i < bus.activity_list.size(); ++i)
				{
					const auto &act = bus.activity_list.at(i);
					constexpr std::array<std::string_view, 2> rw{
						"read",
						"write",
					};
					fmt::print("\ncycle: {}\n", i + 1);
					fmt::print("bus address: {}\n", act.address);
					fmt::print("value: {}\n", act.value);
					fmt::print("operation: {}\n\n", rw[act.type]);
				}

				test_failed = true;
				bad_cycles = true;
			}

			const auto &name = object["name"].get<std::string>();

			for (size_t i = 0; i < bus.activity_list.size(); ++i)
			{
				if (i >= num_cycles)
					break;

				const auto &act = bus.activity_list.at(i);
				const auto &cycle = object["cycles"].at(i);
				constexpr std::array<std::string_view, 2> rw{
					"read",
					"write",
				};

				if ((act.address != cycle[0].get<uint16_t>()) ||
					(act.value != cycle[1].get<uint16_t>()) ||
					(rw[act.type] != cycle[2].get<std::string>()))
				{
					fmt::print("\ncycle: {}\n", i + 1);
					fmt::print("bus address: {} - expected: {}\n", act.address, cycle[0].get<uint16_t>());
					fmt::print("value: {} - expected: {}\n", act.value, cycle[1].get<uint8_t>());
					fmt::print("operation: {}, expected: {}\n", rw[act.type], cycle[2].get<std::string>());
					test_failed = true;
				}
			}

			if (test_failed)
			{
				fmt::print("[{}]\n", name);
				return false;
			}
		}

		return true;
	}

	fmt::print("Cannot find file for test.\n");
	return false;
}

int lda_tests()
{
	if (!test_with_json("v1/a1.json"))
		return 1;
	if (!test_with_json("v1/b1.json"))
		return 1;
	if (!test_with_json("v1/b9.json"))
		return 1;
	if (!test_with_json("v1/bd.json"))
		return 1;
	if (!test_with_json("v1/ad.json"))
		return 1;
	if (!test_with_json("v1/b5.json"))
		return 1;
	if (!test_with_json("v1/a5.json"))
		return 1;
	if (!test_with_json("v1/a9.json"))
		return 1;
	return 0;
}

int ldx_tests()
{
	if (!test_with_json("v1/a2.json"))
		return 1;
	if (!test_with_json("v1/a6.json"))
		return 1;
	if (!test_with_json("v1/b6.json"))
		return 1;
	if (!test_with_json("v1/ae.json"))
		return 1;
	if (!test_with_json("v1/be.json"))
		return 1;
	return 0;
}

int ldy_tests()
{
	if (!test_with_json("v1/a0.json"))
		return 1;
	if (!test_with_json("v1/a4.json"))
		return 1;
	if (!test_with_json("v1/b4.json"))
		return 1;
	if (!test_with_json("v1/ac.json"))
		return 1;
	if (!test_with_json("v1/bc.json"))
		return 1;
	return 0;
}

int eor_tests()
{
	if (!test_with_json("v1/49.json"))
		return 1;
	if (!test_with_json("v1/45.json"))
		return 1;
	if (!test_with_json("v1/55.json"))
		return 1;
	if (!test_with_json("v1/4d.json"))
		return 1;
	if (!test_with_json("v1/5d.json"))
		return 1;
	if (!test_with_json("v1/59.json"))
		return 1;
	if (!test_with_json("v1/41.json"))
		return 1;
	if (!test_with_json("v1/51.json"))
		return 1;
	return 0;
}

int and_tests()
{
	if (!test_with_json("v1/29.json"))
		return 1;
	if (!test_with_json("v1/25.json"))
		return 1;
	if (!test_with_json("v1/35.json"))
		return 1;
	if (!test_with_json("v1/2d.json"))
		return 1;
	if (!test_with_json("v1/3d.json"))
		return 1;
	if (!test_with_json("v1/39.json"))
		return 1;
	if (!test_with_json("v1/21.json"))
		return 1;
	if (!test_with_json("v1/31.json"))
		return 1;
	return 0;
}

int ora_tests()
{
	if (!test_with_json("v1/09.json"))
		return 1;
	if (!test_with_json("v1/05.json"))
		return 1;
	if (!test_with_json("v1/15.json"))
		return 1;
	if (!test_with_json("v1/0d.json"))
		return 1;
	if (!test_with_json("v1/1d.json"))
		return 1;
	if (!test_with_json("v1/19.json"))
		return 1;
	if (!test_with_json("v1/01.json"))
		return 1;
	if (!test_with_json("v1/11.json"))
		return 1;
	return 0;
}

int adc_tests()
{
	if (!test_with_json("v1/69.json"))
		return 1;
	if (!test_with_json("v1/65.json"))
		return 1;
	if (!test_with_json("v1/75.json"))
		return 1;
	if (!test_with_json("v1/6d.json"))
		return 1;
	if (!test_with_json("v1/7d.json"))
		return 1;
	if (!test_with_json("v1/79.json"))
		return 1;
	if (!test_with_json("v1/61.json"))
		return 1;
	if (!test_with_json("v1/71.json"))
		return 1;
	return 0;
}

int sbc_tests()
{
	if (!test_with_json("v1/e9.json"))
		return 1;
	if (!test_with_json("v1/e5.json"))
		return 1;
	if (!test_with_json("v1/f5.json"))
		return 1;
	if (!test_with_json("v1/ed.json"))
		return 1;
	if (!test_with_json("v1/fd.json"))
		return 1;
	if (!test_with_json("v1/f9.json"))
		return 1;
	if (!test_with_json("v1/e1.json"))
		return 1;
	if (!test_with_json("v1/f1.json"))
		return 1;
	return 0;
}

int cmp_tests()
{
	if (!test_with_json("v1/c9.json"))
		return 1;
	if (!test_with_json("v1/c5.json"))
		return 1;
	if (!test_with_json("v1/d5.json"))
		return 1;
	if (!test_with_json("v1/cd.json"))
		return 1;
	if (!test_with_json("v1/dd.json"))
		return 1;
	if (!test_with_json("v1/d9.json"))
		return 1;
	if (!test_with_json("v1/c1.json"))
		return 1;
	if (!test_with_json("v1/d1.json"))
		return 1;
	return 0;
}

int cpx_tests()
{
	if (!test_with_json("v1/e0.json"))
		return 1;
	if (!test_with_json("v1/e4.json"))
		return 1;
	if (!test_with_json("v1/ec.json"))
		return 1;
	return 0;
}

int cpy_tests()
{
	if (!test_with_json("v1/c0.json"))
		return 1;
	if (!test_with_json("v1/c4.json"))
		return 1;
	if (!test_with_json("v1/cc.json"))
		return 1;
	return 0;
}

int bit_tests()
{
	if (!test_with_json("v1/24.json"))
		return 1;
	if (!test_with_json("v1/2c.json"))
		return 1;
	return 0;
}

int sta_tests()
{
	if (!test_with_json("v1/85.json"))
		return 1;
	if (!test_with_json("v1/95.json"))
		return 1;
	if (!test_with_json("v1/8d.json"))
		return 1;
	if (!test_with_json("v1/9d.json"))
		return 1;
	if (!test_with_json("v1/99.json"))
		return 1;
	if (!test_with_json("v1/81.json"))
		return 1;
	if (!test_with_json("v1/91.json"))
		return 1;
	return 0;
}

int stx_tests()
{
	if (!test_with_json("v1/86.json"))
		return 1;
	if (!test_with_json("v1/96.json"))
		return 1;
	if (!test_with_json("v1/8e.json"))
		return 1;
	return 0;
}

int sty_tests()
{
	if (!test_with_json("v1/84.json"))
		return 1;
	if (!test_with_json("v1/94.json"))
		return 1;
	if (!test_with_json("v1/8c.json"))
		return 1;
	return 0;
}

int inc_tests()
{
	if (!test_with_json("v1/e6.json"))
		return 1;
	if (!test_with_json("v1/f6.json"))
		return 1;
	if (!test_with_json("v1/ee.json"))
		return 1;
	if (!test_with_json("v1/fe.json"))
		return 1;
	if (!test_with_json("v1/e8.json"))
		return 1;
	if (!test_with_json("v1/c8.json"))
		return 1;
	return 0;
}

int dec_tests()
{
	if (!test_with_json("v1/c6.json"))
		return 1;
	if (!test_with_json("v1/d6.json"))
		return 1;
	if (!test_with_json("v1/ce.json"))
		return 1;
	if (!test_with_json("v1/de.json"))
		return 1;
	if (!test_with_json("v1/ca.json"))
		return 1;
	if (!test_with_json("v1/88.json"))
		return 1;
	return 0;
}

int flag_tests()
{
	if (!test_with_json("v1/18.json"))
		return 1;
	if (!test_with_json("v1/d8.json"))
		return 1;
	if (!test_with_json("v1/58.json"))
		return 1;
	if (!test_with_json("v1/b8.json"))
		return 1;
	if (!test_with_json("v1/38.json"))
		return 1;
	if (!test_with_json("v1/f8.json"))
		return 1;
	if (!test_with_json("v1/78.json"))
		return 1;

	return 0;
}

int transfer_tests()
{
	if (!test_with_json("v1/aa.json"))
		return 1;
	if (!test_with_json("v1/a8.json"))
		return 1;
	if (!test_with_json("v1/ba.json"))
		return 1;
	if (!test_with_json("v1/8a.json"))
		return 1;
	if (!test_with_json("v1/9a.json"))
		return 1;
	if (!test_with_json("v1/98.json"))
		return 1;

	return 0;
}

int stack_tests()
{
	if (!test_with_json("v1/48.json"))
		return 1;
	if (!test_with_json("v1/08.json"))
		return 1;
	if (!test_with_json("v1/68.json"))
		return 1;
	if (!test_with_json("v1/28.json"))
		return 1;
	return 0;
}

int asl_tests()
{
	if (!test_with_json("v1/0a.json"))
		return 1;
	if (!test_with_json("v1/06.json"))
		return 1;
	if (!test_with_json("v1/16.json"))
		return 1;
	if (!test_with_json("v1/0e.json"))
		return 1;
	if (!test_with_json("v1/1e.json"))
		return 1;
	return 0;
}

int lsr_tests()
{
	if (!test_with_json("v1/4a.json"))
		return 1;
	if (!test_with_json("v1/46.json"))
		return 1;
	if (!test_with_json("v1/56.json"))
		return 1;
	if (!test_with_json("v1/4e.json"))
		return 1;
	if (!test_with_json("v1/5e.json"))
		return 1;
	return 0;
}

int rol_tests()
{
	if (!test_with_json("v1/2a.json"))
		return 1;
	if (!test_with_json("v1/26.json"))
		return 1;
	if (!test_with_json("v1/36.json"))
		return 1;
	if (!test_with_json("v1/2e.json"))
		return 1;
	if (!test_with_json("v1/3e.json"))
		return 1;
	return 0;
}

int ror_tests()
{
	if (!test_with_json("v1/6a.json"))
		return 1;
	if (!test_with_json("v1/66.json"))
		return 1;
	if (!test_with_json("v1/76.json"))
		return 1;
	if (!test_with_json("v1/6e.json"))
		return 1;
	if (!test_with_json("v1/7e.json"))
		return 1;
	return 0;
}

int branch_tests()
{
	if (!test_with_json("v1/90.json"))
		return 1;
	if (!test_with_json("v1/b0.json"))
		return 1;
	if (!test_with_json("v1/f0.json"))
		return 1;
	if (!test_with_json("v1/30.json"))
		return 1;
	if (!test_with_json("v1/d0.json"))
		return 1;
	if (!test_with_json("v1/10.json"))
		return 1;
	if (!test_with_json("v1/50.json"))
		return 1;
	if (!test_with_json("v1/70.json"))
		return 1;
	return 0;
}

int jump_tests()
{
	if (!test_with_json("v1/6c.json"))
		return 1;
	if (!test_with_json("v1/4c.json"))
		return 1;
	if (!test_with_json("v1/60.json"))
		return 1;
	if (!test_with_json("v1/20.json"))
		return 1;
	if (!test_with_json("v1/40.json"))
		return 1;
	if (!test_with_json("v1/00.json"))
		return 1;
	return 0;
}

int main()
{
	fmt::print("Starting Tests.\n");

	// NOP
	if (!test_with_json("v1/ea.json"))
		return 1;
	if (jump_tests())
		return 1;

	// ---

	if (branch_tests())
		return 1;

	if (ror_tests())
		return 1;
	if (rol_tests())
		return 1;
	if (lsr_tests())
		return 1;
	if (asl_tests())
		return 1;
	if (stack_tests())
		return 1;
	if (transfer_tests())
		return 1;
	if (flag_tests())
		return 1;
	if (dec_tests())
		return 1;
	if (inc_tests())
		return 1;
	if (sty_tests())
		return 1;
	if (stx_tests())
		return 1;
	if (sta_tests())
		return 1;
	if (bit_tests())
		return 1;
	if (cpy_tests())
		return 1;
	if (cpx_tests())
		return 1;
	if (cmp_tests())
		return 1;
	if (sbc_tests())
		return 1;
	if (adc_tests())
		return 1;
	if (ora_tests())
		return 1;
	if (and_tests())
		return 1;
	if (eor_tests())
		return 1;
	if (ldy_tests())
		return 1;
	if (ldx_tests())
		return 1;
	if (lda_tests())
		return 1;
	fmt::print("All Complete.\n");
	return 0;
}