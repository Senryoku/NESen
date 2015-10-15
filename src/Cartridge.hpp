#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <functional>

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
		delete _chr_ram;
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
		
		if(_chr_rom_size == 0) // CHR RAM
		{
			_use_chr_ram = true;
			_chr_ram_size = 128 * 1024; /// @todo TEMP
			_chr_ram = new byte_t[_chr_ram_size];
		} else {
			_chr_rom = new byte_t[_chr_rom_size];
			file.read(_chr_rom, _chr_rom_size);
		}
		
		_prg_ram = new byte_t[_prg_ram_size];
		
		_mapper = ((flag6 & 0b11110000) >> 4) | (h[7] & 0b11110000);
		
		std::cout << "Loaded '" << path << "' successfully! " << std::endl << 
					"> Mapper: " << _mapper << std::endl <<
					"> PRG Cartridge size: " << 16 * h[4] << "kB (" << _prg_rom_size << "B)" << std::endl <<
					"> CHR Cartridge size: " << 8 * h[5] << "kB (" << _chr_rom_size << "B)" << std::endl <<
					"> PRG RAM size: " << 8 * h[8] << "kB (" << _prg_ram_size << "B)" << std::endl;
		
		return true;
	}
	
	/// CPU Read
	byte_t read(addr_t addr) const
	{
		return _mappers_read[_mapper](addr);
	}
	
	/// CPU Write
	void write(addr_t addr, word_t value)
	{
		_mappers_write[_mapper](addr, value);
	}
	
	/// PPU Read
	byte_t read_chr(addr_t addr)
	{
		if(_use_chr_ram)
			return _mappers_read_chr_ram[_mapper](addr);
		else
			return _mappers_read_chr[_mapper](addr);
	}

private:
	word_t	_control_register = 0;
	word_t	_shift_register = 0;
	size_t	_shift_register_writes = 0;

	size_t	_prg_rom_size = 0;
	size_t	_chr_rom_size = 0;
	size_t	_prg_ram_size = 0;
	size_t	_chr_ram_size = 0;
	bool	_use_chr_ram = false;
	
	size_t _chr_rom_banks[8] = {0};
	size_t _chr_ram_banks[8] = {0};
	size_t _prg_rom_banks[8] = {0};
	
	size_t	_mapper = 0;
		
	byte_t*	_trainer = nullptr;
	byte_t*	_prg_rom = nullptr;
	byte_t*	_chr_rom = nullptr;
	byte_t*	_chr_ram = nullptr;
	byte_t*	_prg_ram = nullptr;
	
	
	std::function<byte_t(addr_t)> _mappers_read[0x100] = {
		[&] (addr_t addr) -> byte_t	// 000
		{
			if(addr >= 0x6000 && addr < 0x8000)
			{
				return _prg_ram[(addr - 0x6000) % _prg_ram_size];
			} else if(addr >= 0x8000 && addr < 0xC000) {			// First 16 KB of ROM.
				return _prg_rom[(addr - 0x8000) % _prg_rom_size];
			} else if(addr >= 0xC000) {								// Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				if(_prg_rom_size > 0x4000)
					return _prg_rom[(addr - 0x8000)];
				else
					return _prg_rom[(addr - 0xC000)];
			}
			std::cerr << "Error: Trying to read Cartridge at address " << Hexa(addr) << std::endl;
			return 0;
		},
		[&] (addr_t addr) -> byte_t	// 001
		{
			if(addr >= 0x6000 && addr < 0x8000)
			{
				return _prg_ram[(addr - 0x6000) % _prg_ram_size];
			} else if(addr >= 0x8000 && addr < 0xC000) {
				return _prg_rom[(addr - 0x8000) % _prg_rom_size + (_prg_rom_banks[0] & 0xF) * 0x4000];
			} else if(addr >= 0xC000) {
				return _prg_rom[(addr - 0x8000) % _prg_rom_size + (_prg_rom_banks[1] & 0xF) * 0x4000];
			}
			std::cerr << "Error: Trying to read Cartridge at address " << Hexa(addr) << std::endl;
			return 0;
		} 
	};
	
	std::function<void(addr_t, word_t)> _mappers_write[0x100] = {
		[&] (addr_t addr, word_t value) -> void	// 000
		{
			std::cerr << "Error: Trying to write Cartridge (mapper " << _mapper << ") at address " << Hexa(addr) << std::endl;
		},
		[&] (addr_t addr, word_t value) -> void	// 001
		{
			if(addr >= 0x8000)
			{
				if(value & 0x80)
				{
					_shift_register = 0;
					_shift_register_writes = 0;
				} else {
					_shift_register |= ((value & 1) << _shift_register_writes);
					_shift_register_writes++;
					if(_shift_register_writes == 5)
					{
						switch(addr & 0xC000)
						{
							case 0x8000: _control_register = _shift_register; break;
							case 0xA000: 
								if(_use_chr_ram)
									_chr_ram_banks[0] = _shift_register;
								else
									_chr_rom_banks[0] = _shift_register;
								break;
							case 0xC000: 
								if(_use_chr_ram)
									_chr_ram_banks[1] = _shift_register;
								else
									_chr_rom_banks[1] = _shift_register;
								break;
							case 0xE000: _prg_rom_banks[0] = _shift_register; break;
						}
						_shift_register = 0;
						_shift_register_writes = 0;
					}
				}
			}
		}
	};
	
	std::function<byte_t(addr_t)> _mappers_read_chr[0x100] = {
		[&] (addr_t addr) -> byte_t	// 000
		{
			return _chr_rom[addr % _chr_rom_size];
		},
		[&] (addr_t addr) -> byte_t	// 001
		{
			if(addr < 0x1000)
				return _chr_rom[addr % _chr_rom_size + 0x1000 * _chr_rom_banks[0]];
			else
				return _chr_rom[addr % _chr_rom_size + 0x1000 * _chr_rom_banks[1]];
		}
	};
	
	std::function<byte_t(addr_t)> _mappers_read_chr_ram[0x100] = {
		[&] (addr_t addr)	// 000
		{
			return _chr_ram[addr % _chr_ram_size];
		},
		[&] (addr_t addr)	// 001
		{
			if(addr < 0x1000)
				return _chr_ram[addr % _chr_ram_size + 0x1000 * _chr_ram_banks[0]];
			else
				return _chr_ram[addr % _chr_ram_size + 0x1000 * _chr_ram_banks[1]];
		}
	};
};
