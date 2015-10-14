#pragma once

#include <string>
#include <iostream>
#include <fstream>

/**
 * NES Cartridge
**/
class Cartridge
{
public:
	using byte_t = char;
	using addr_t = uint16_t;
	
	Cartridge()
	{
	}
	
	Cartridge(const std::string& path)
	{
		load(path);
	}
	
	~Cartridge()
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
		
		_mapper = ((flag6 & 0b11110000) >> 4) & (h[7] & 0b11110000);
		
		std::cout << "Loaded '" << path << "' successfully! " << std::endl << 
					"> Mapper: " << _mapper << std::endl <<
					"> PRG Cartridge size: " << 16 * h[4] << "kB (" << _prg_rom_size << "B)" << std::endl <<
					"> CHR Cartridge size: " << 8 * h[5] << "kB (" << _chr_rom_size << "B)" << std::endl <<
					"> PRG RAM size: " << 8 * h[8] << "kB (" << _prg_ram_size << "B)" << std::endl;
		
		return true;
	}
	
	byte_t read(addr_t addr) const
	{
		// Mapper 00
		if(addr >= 0x6000 && addr < 0x8000)
			return _prg_ram[(addr - 0x6000) % _prg_ram_size];
		else if(addr >= 0x8000 && addr < 0xC000)				// First 16 KB of ROM.
			return _prg_rom[(addr - 0x8000) % _prg_rom_size];
		else if(addr >= 0xC000)									// Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
			return _prg_rom[(addr - 0x8000) % _prg_rom_size];
			
		std::cerr << "Error: Trying to read Cartridge at address 0x" << std::hex << addr << std::endl;
		exit(1);
		return _prg_rom[0];
	}
	
	byte_t read_chr(addr_t addr)
	{
		return _chr_rom[addr % _chr_rom_size];
	}

private:
	size_t	_prg_rom_size = 0;
	size_t	_chr_rom_size = 0;
	size_t	_prg_ram_size = 0;
	
	int		_mapper = 0;
		
	byte_t*	_trainer = nullptr;
	byte_t*	_prg_rom = nullptr;
	byte_t*	_chr_rom = nullptr;
	byte_t*	_prg_ram = nullptr;
};
