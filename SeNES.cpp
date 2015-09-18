#include "NES.hpp"

int main(int argc, char* argv[])
{
	std::string path = "nestest.nes";
	
	if(argc > 1)
		path = argv[1];
	
	NES nes;
	if(!nes.load(path))
	{
		std::cerr << "Error loading '" << path << "'. Exiting..." << std::endl;
		return 0;
	}
	
	//nes.cpu.setTestLog("nestest_addr.log");
	nes.run();
}
