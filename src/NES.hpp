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
	
	void reset()
	{
		cpu.reset();
		ppu.reset();
	}
	
	void run()
	{
		reset();
		while(!_shutdown)
		{
			step();
		}
	}
	
	void step()
	{
		cpu.step();
		ppu.step(cpu.getCycles());
	}
	
private:
	bool	_shutdown = false;
	float	_ppucpuRatio = 3.0;
};
