#include <fmt/format.h>
#include <nes/core.hpp>
#include <nes/cartridge.hpp>
int main()
{
	fmt::println("Hello World");
	using namespace NESterpiece;

	NROM rom;
	rom.from_file("Donkey Kong (World) (Rev 1).nes");
	return 0;
}