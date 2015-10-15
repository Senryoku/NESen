#pragma once

#include <cassert>
#include <cstring>
#include <fstream>
#include <vector>

#include "Common.hpp"
#include "Cartridge.hpp"

/**
 * NES Picture Processing Unit
 *
 *
**/ 
class PPU
{
public:	
	const size_t ScreenWidth = 256;
	const size_t ScreenHeight = 240;
	const size_t CyclesPerScanline = 240;
	
	const size_t MemSize = 0x4000;
	const size_t OAMSize = 0x100;
	
	enum PPUControl : word_t
	{
		NameTableAddress = 0x03,
		VerticalWrite = 0x04,
		SpritePatternTableAddress = 0x08,
		BackgoundPatternTableAddress = 0x10,
		SpriteSize = 0x20,
		PPUMasterSlaveMode = 0x40,
		VBlankEnable = 0x80
	};
	
	enum PPUMask : word_t
	{
		Grayscale = 0x01,
		LeftBackgroundMask = 0x02,
		LeftSpriteMask = 0x04,
		BackgroundMask = 0x08,
		SpriteMask = 0x10,
		EmphasizeRed = 0x20,
		EmphasizeGreen = 0x40,
		EmphasizeBlue = 0x80
	};
	
	enum PPUStatus : word_t
	{
		SpriteOverflow = 0x20,
		Sprite0Hit = 0x40,
		VBlank = 0x80
	};
	
	enum SpriteAttribute
	{
		Palette		= 0x03,
		Priority 	= 0x20,
		FlipX	 	= 0x40,
		FlipY	 	= 0x80
	};
	
	Cartridge* cartridge = nullptr;
	
	PPU() :
		_mem(new word_t[MemSize]),
		_oam(new word_t[OAMSize]),
		_screen(new color_t[240 * 256])
	{
		reset();
	}
	
	~PPU()
	{
		delete[] _screen;
		delete[] _oam;
		delete[] _mem;
	}
	
	bool load_palette(const std::string& path)
	{
		std::ifstream pal_file(path, std::ios::binary);
		if(!pal_file)
		{
			std::cerr << "Error: Couldn't load palette at '" << path << "'." << std::endl;
			return false;
		} else {
			word_t r, b, g;
			size_t c = 0;
			while(pal_file)
			{
				pal_file >> r;
				pal_file >> g;
				pal_file >> b;
				rgb_palette[c] = color_t(r, g, b);
				++c;
			}
			return true;
		}
	}
	
	void reset()
	{
		std::memset(_screen, 0, 240 * 256);
		std::memset(_mem, 0, MemSize);
	}
	
	/// Access for CPU
	inline word_t read(addr_t addr)
	{
		if(addr < 0x2000) // CHR ROM (Or re-routed by cartridge)
			return cartridge->read_chr(addr);
		
		assert(addr < 0x4000);
		switch(addr & 0x7)
		{
			case 0x00: return _ppu_control;
			case 0x01: return _ppu_mask;
			case 0x02: 
			{
				word_t r = _ppu_status;
				_ppu_status &= ~VBlank; // Clear VBlank Status when reading it
				return r;
			}
			case 0x04: return _oam[_oam_addr];
			case 0x07: 
			{
				word_t r = _mem[_ppu_addr];
				_ppu_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
				return r;
			}
		}
		std::cerr << "Read on unsupported PPU register: " << Hexa(addr) << std::endl;
		return 0;
	}
	
	/// Write from CPU
	inline void write(addr_t addr, word_t value)
	{
		assert(addr >= 0x2000 && addr < 0x4000);
		switch(addr & 0x7)
		{
			case 0x00: _ppu_control = value; break;
			case 0x01: _ppu_mask = value; break;
			case 0x02: _ppu_status = value; break;
			case 0x03: _oam_addr = value; break;
			case 0x04: _oam[_oam_addr++] = value; break;
			case 0x05: // Scrolling Register
				if(_scroll_first_read)
					_scroll_x = value;
				else
					_scroll_y = value;
				_scroll_first_read = !_scroll_first_read;
			break;
			case 0x06:
				if(_ppu_addr_upper)
					_ppu_addr = (_ppu_addr & 0x00FF) | (value << 8);
				else
					_ppu_addr = (_ppu_addr & 0xFF00) | value;
				_ppu_addr_upper = !_ppu_addr_upper;
			break;
			case 0x07:
				_mem[_ppu_addr] = value;
				//std::cout << "PPU Write: " << Hexa(_ppu_addr) << " = " << Hexa8(value) << std::endl;
				_ppu_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
			break;
			default:
				std::cerr << "Write on unsupported PPU address: " << Hexa(addr) << std::endl;
			break;
		}
	}
	
	inline const color_t* get_screen() const
	{
		return _screen;
	}

	color_t rgb_palette[0x40]
	= {
		color_t(84, 84, 84),	color_t(0, 30, 116),	color_t(8, 16, 144),	color_t(48, 0, 136),	color_t(68, 0, 100),	color_t(92, 0, 48),		color_t(84, 4, 0),		color_t(60, 24, 0),		color_t(32, 42, 0),		color_t(8, 58, 0), 		color_t(0, 64, 0), 		color_t(0, 60, 0), 		color_t(0, 50, 60), 	color_t(0, 0, 0), 		color_t(0, 0, 0), color_t(0, 0, 0),
		color_t(152, 150, 152),	color_t(8, 76, 196),	color_t(48, 50, 236),	color_t(92, 30, 228),	color_t(136, 20, 176),	color_t(160, 20, 100),	color_t(152, 34, 32),	color_t(120, 60, 0), 	color_t(84, 90, 0), 	color_t(40, 114, 0), 	color_t(8, 124, 0), 	color_t(0, 118, 40), 	color_t(0, 102, 120), 	color_t(0, 0, 0), 		color_t(0, 0, 0), color_t(0, 0, 0),
		color_t(236, 238, 236),	color_t(76, 154, 236),	color_t(120, 124, 236),	color_t(176, 98, 236),	color_t(228, 84, 236),	color_t(236, 88, 180),	color_t(236, 106, 100),	color_t(212, 136, 32), 	color_t(160, 170, 0), 	color_t(116, 196, 0), 	color_t(76, 208, 32), 	color_t(56, 204, 108), 	color_t(56, 180, 204), 	color_t(60, 60, 60), 	color_t(0, 0, 0), color_t(0, 0, 0),
		color_t(236, 238, 36),	color_t(168, 204, 236),	color_t(188, 188, 236),	color_t(212, 178, 236),	color_t(236, 174, 236),	color_t(236, 174, 212),	color_t(236, 180, 176),	color_t(228, 196, 144), color_t(204, 210, 120), color_t(180, 222, 120), color_t(168, 226, 144), color_t(152, 226, 180), color_t(160, 214, 228), color_t(160, 162, 160), color_t(0, 0, 0), color_t(0, 0, 0)
	};
	
	void step(size_t cpu_cycles);
	
	static inline void palette_translation(word_t l, word_t h, word_t& r0, word_t& r1)
	{
		r0 = r1 = 0;
		for(int i = 0; i < 4; i++)
			r0 |= (((l & (1 << (i + 4))) >> (4 + i)) << (2 * i)) | 
					(((h & (1 << (i + 4))) >> (4 + i))  << (2 * i + 1));
		for(int i = 0; i < 4; i++)
			r1 |= ((l & (1 << i)) << (2 * i - i)) | 
					((h & (1 << i)) << (2 * i + 1 - i));
	}
	
	bool completed_frame = false;
	
	inline bool check_nmi()
	{
		bool r = _nmi;
		_nmi = false;
		return r;
	}
	
	inline word_t get_mem(addr_t addr) const { return _mem[addr]; }
	
private:
	unsigned int _cycles = 0;
	unsigned int _frame = 0;
	unsigned int _line	= 261;
	unsigned int _dot	= 0;
	
	// Registers
	word_t		_ppu_control = 0;			// $2000
	word_t		_ppu_mask = 0;				// $2001
	word_t		_ppu_status = 0;			// $2002
	word_t		_oam_addr = 0;				// $2003
	bool		_scroll_first_read = true;
	word_t		_scroll_x = 0;				// Access by $2005
	word_t		_scroll_y = 0;				// Access by $2005
	
	bool 		_nmi = false; 				// Non-Maskable Interrupt 
	
	bool		_ppu_addr_upper = true;
	addr_t		_ppu_addr = 0;				// Access by $2006
	
	word_t*		_mem = nullptr;				// Access by $2007
	word_t*		_oam = nullptr;				// Access by $2004
	
	color_t*	_screen;
};