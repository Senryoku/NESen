#include "Cartridge.hpp"

Cartridge::Cartridge(const std::string& path)
{
	load(path);
}

Cartridge::~Cartridge()
{
	delete _trainer;
	delete _prg_rom;
	delete _chr_rom;
	delete _prg_ram;
	delete _chr_ram;
}
	
bool Cartridge::load(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	
	if(!file)
	{
		Log::error("Error: '", path, "' could not be opened.");
		return false;
	}
	
	byte_t h[16]; // header
	file.read(h, 16);

	if(!(h[0] == 0x4E && h[1] == 0x45 && h[2] == 0x53 && h[3] == 0x1A))
	{
		Log::error("Error: '", path, "' is not a valid iNES file (wrong header).");
		return false;
	}
	
	_prg_rom_size = 16384 * static_cast<size_t>(h[4]);
	_chr_rom_size = 8192 * static_cast<size_t>(h[5]);
	_prg_ram_size = 8192 * static_cast<size_t>(h[8]);
	
	// Infers 8kb PRG RAM for compatibility.
	if(_prg_ram_size == 0)
		_prg_ram_size = 8192;
	
	char flag6 = h[6];
	// Trainer
	if(flag6 & 0b00000100)
	{
		_trainer = new byte_t[512];
		file.read(_trainer, 512);
	}
	
	if(flag6 & 0x8)
		_mirrorring = None;
	else
		_mirrorring = (flag6 & 1) ? Vertical : Horizontal;
	
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
	
	if(!_mappers_read_chr[_mapper])
	{
		Log::error("Error: mapper ", _mapper, " is not supported.");
		return false;
	}
	
	Log::info("Loaded '", path, "' successfully! "); 
	Log::info("> Mapper: ", _mapper, ", Mirroring: ", ((_mirrorring == None) ? "None" : (_mirrorring == Vertical ? "Vertical" : "Horizontal"))); 
	Log::info("> PRG Cartridge size: ", 16 * h[4], "kB (", _prg_rom_size, "B)"); 
	Log::info("> CHR Cartridge size: ", 8 * h[5], "kB (", _chr_rom_size, "B)"); 
	Log::info("> PRG RAM size: ", 8 * h[8], "kB (", _prg_ram_size, "B)"); 
	
	return true;
}
