#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <functional>

#include "Common.hpp"

/**
 * NES Cartridge
**/
class Cartridge
{
public:
	/// Nametables mirroring
	enum Mirroring
	{
		Horizontal,
		Vertical,
		None		// Four Screens
	};

	Cartridge() =default;
	Cartridge(const std::string& path);
	~Cartridge();
	
	bool load(const std::string& path);
	
	inline Mirroring get_mirroring() const { return _mirrorring; }
	
	/// CPU Read
	inline byte_t read(addr_t addr) const
	{
		assert(_mappers_read[_mapper]);
		return _mappers_read[_mapper](addr);
	}
	
	/// CPU Write
	inline void write(addr_t addr, word_t value)
	{
		assert(_mappers_write[_mapper]);
		_mappers_write[_mapper](addr, value);
	}
	
	/// PPU Read
	inline byte_t read_chr(addr_t addr)
	{
		if(_use_chr_ram)
		{
			assert(_mappers_read_chr_ram[_mapper]);
			return _mappers_read_chr_ram[_mapper](addr);
		} else {
			assert(_mappers_read_chr[_mapper]);
			return _mappers_read_chr[_mapper](addr);
		}
	}

private:
	Mirroring	_mirrorring;

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
	
	inline void read_error(addr_t addr) const
	{
		std::cerr << "Error: Trying to read Cartridge (mapper: " 
					<< _mapper << ") at address " << Hexa(addr) << std::endl;
	}
	
	inline void write_error(addr_t addr) const
	{
		std::cerr << "Error: Trying to write Cartridge (mapper: " 
					<< _mapper << ") at address " << Hexa(addr) << std::endl;
	}
	
	std::function<byte_t(addr_t)> _mappers_read[0x100] = {
		[&] (addr_t addr) -> byte_t	// 000
		{
			if(addr >= 0x6000 && addr < 0x8000)
			{
				return _prg_ram[(addr - 0x6000) % _prg_ram_size];
			} else if(addr >= 0x8000 && addr < 0xC000) {			// First 16 KB of ROM.
				return _prg_rom[addr - 0x8000];
			} else if(addr >= 0xC000) {								// Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
				if(_prg_rom_size > 0x4000)
					return _prg_rom[addr - 0x8000];
				else
					return _prg_rom[addr - 0xC000];
			}
			read_error(addr);
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
			read_error(addr);
			return 0;
		} 
	};
	
	std::function<void(addr_t, word_t)> _mappers_write[0x100] = {
		[&] (addr_t addr, word_t value) -> void	// 000
		{
			write_error(addr);
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
			write_error(addr);
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
