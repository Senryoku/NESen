#pragma once

#include <string>
#include <iostream>
#include <fstream>

/**
 * NES Cartridge
**/
class ROM
{
public:
	using byte_t = char;
	
	ROM()
	{
	}
	
	ROM(const std::string& path)
	{
		load(path);
	}
	
	~ROM()
	{
		delete _trainer;
		delete _prg_rom;
		delete _chr_rom;
		delete _prg_ram;
	}
	
	bool load(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary);
		
		if(!file)
		{
			std::cerr << "Error: '" << path << "' could not be opened." << std::endl;
			return false;
		}
		
		byte_t h[16]; // header
		file.read(h, 16);

		if(!(h[0] == 0x4E && h[1] == 0x45 && h[2] == 0x53 && h[3] == 0x1A))
		{
			std::cerr << "Error: '" << path << "' is not a valid iNES file (wrong header)." << std::endl;
			return false;
		}
		
		_prg_rom_size = 16384 * static_cast<size_t>(h[4]);
		_chr_rom_size = 8192 * static_cast<size_t>(h[5]);
		_prg_ram_size = 8192 * static_cast<size_t>(h[8]);
		
		char flag6 = h[6];
		// Trainer
		if(flag6 & 0b00000100)
		{
			_trainer = new byte_t[512];
			file.read(_trainer, 512);
		}
			
		_prg_rom = new byte_t[_prg_rom_size];
		file.read(_prg_rom, _prg_rom_size);
		
		_chr_rom = new byte_t[_chr_rom_size];
		file.read(_chr_rom, _chr_rom_size);
		
		_prg_ram = new byte_t[_prg_ram_size];
		
		std::cout << "Loaded '" << path << "' successfully! " << std::endl << 
					"> PRG ROM size: " << static_cast<size_t>(h[4]) << "kB (" << _prg_rom_size << "B)" << std::endl <<
					"> CHR ROM size: " << static_cast<size_t>(h[5]) << "kB (" << _chr_rom_size << "B)" << std::endl <<
					"> PRG RAM size: " << static_cast<size_t>(h[8]) << "kB (" << _prg_ram_size << "B)" << std::endl;
		
		return true;
	}
	
	byte_t read(uint16_t addr)
	{
		return _prg_rom[addr % _prg_rom_size];
	}

private:
	size_t	_prg_rom_size = 0;
	size_t	_chr_rom_size = 0;
	size_t	_prg_ram_size = 0;
		
	byte_t*	_trainer = nullptr;
	byte_t*	_prg_rom = nullptr;
	byte_t*	_chr_rom = nullptr;
	byte_t*	_prg_ram = nullptr;
};
