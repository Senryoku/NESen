#include "CPU.hpp"

int main(int argc, char* argv[])
{
	std::string path = "nestest.nes";
	
	if(argc > 1)
		path = argv[1];
	
	ROM rom;
	if(!rom.load(path))
	{
		std::cerr << "Error loading '" << path << "'. Exiting..." << std::endl;
		return 0;
	}
	
	PPU ppu;
	CPU cpu;
	cpu.rom = &rom;
	cpu.ppu = &ppu;
	cpu.run();
}
