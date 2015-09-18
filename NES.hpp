#pragma once

#include "CPU.hpp"

class NES
{
public:
	CPU cpu;
	APU apu;
	PPU ppu;
	ROM rom;
	
	NES()
	{
		init();
	}
	
	NES(const std::string& path)
	{
		init();
		load(path);
	}
	
	~NES()
	{
	}
	
	void init()
	{
		cpu.ppu = &ppu;
		cpu.rom = &rom;
		cpu.apu = &apu;
	}
	
	bool load(const std::string& path)
	{
		return rom.load(path);
	}
	
	void run()
	{
		cpu.reset();
		while(!_shutdown)
		{
			cpu.step();
			//while(cpu.getCycles() > ppu.getCycles() * _ppucpuRatio)
				ppu.step();
		}
	}
	
private:
	bool	_shutdown = false;
	float	_ppucpuRatio = 3.0;
};
