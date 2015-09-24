#pragma once

#include "Cartridge.hpp"

/**
 * NES Picture Processing Unit
 *
 *
**/ 
class PPU
{
public:
	using word_t = uint8_t;
	using addr_t = uint16_t;
	struct color_t
	{ 
		word_t r = 0, g = 0, b = 0, a = 0;
		
		color_t& operator=(const color_t& v) =default;
		inline bool operator==(const color_t& v)
		{
			return r == v.r && g == v.g && b == v.b && a == v.a;
		}
		
		inline color_t& operator=(word_t val)
		{
			r = g = b = a = val;
			return *this;
		}
		
		inline bool operator==(word_t val)
		{
			return r == g && g == b && b == a && a == val;
		}
	};
	
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
		
		switch(addr)
		{
			case 0x2000: return _ppu_control;
			case 0x2001: return _ppu_mask;
			case 0x2002: return _ppu_status;
			case 0x2004: return _oam[_oam_addr];
			case 0x2007: 
				_ppu_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
				return _mem[_ppu_addr];
		}
		std::cerr << "Read on unsupported PPU register: 0x" << std::hex << addr << std::endl;
		return 0;
	}
	
	/// Write from CPU
	inline void write(addr_t addr, word_t value)
	{
		switch(addr)
		{
			case 0x2000: _ppu_control = value; break;
			case 0x2001: _ppu_mask = value; break;
			case 0x2002: _ppu_status = value; break;
			case 0x2003: _oam_addr = value; break;
			case 0x2004: _oam[_oam_addr++] = value; break;
			case 0x2005: // Scrolling Register
				if(_scroll_first_read)
					_scroll_x = value;
				else
					_scroll_y = value;
				_scroll_first_read = !_scroll_first_read;
			break;
			case 0x2006:
				if(_ppu_addr_upper)
					_ppu_addr = (_ppu_addr & 0x00FF) | (value << 8);
				else
					_ppu_addr = (_ppu_addr & 0xFF00) | value;
				_ppu_addr_upper = !_scroll_first_read;
			break;
			case 0x2007:
				_mem[_ppu_addr] = value;
				//std::cout << "PPU Write: 0x" << std::hex << _ppu_addr << std::endl;
				_ppu_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
			break;
			default:
				std::cerr << "Write on unsupported PPU address: 0x" << std::hex << addr << std::endl;
			break;
		}
	}
	
	inline const color_t* get_screen() const
	{
		return _screen;
	}
	
	void step(size_t cpu_cycles)
	{
		_cycles += 3 * cpu_cycles;
		if(_cycles > CyclesPerScanline)
		{
			if(_line < ScreenHeight)
			{
				word_t t;
				word_t tile_l;
				word_t tile_h;
				word_t tile_data0 = 0, tile_data1 = 0;
				word_t y = (_scroll_y + _line) & 3;
				word_t sprite_pixel = _scroll_x & 3;
				addr_t nametable = 0x2000 + 0x400 * (_ppu_control & NameTableAddress);
				addr_t patterns = (_ppu_control & BackgoundPatternTableAddress) ? 0x1000 : 0;
				nametable += (_scroll_x >> 3) + 32 * ((_scroll_y + _line) >> 3);
				//word_t palette = 0;
				for(size_t x = 0; x < ScreenWidth; ++x)
				{
					// Fetch Tile Data
					if(sprite_pixel == 8 || x == 0)
					{
						t = _mem[nametable + ((x + _scroll_x) >> 3)];
						sprite_pixel = sprite_pixel % 8;
						tile_l = read(patterns + t * 16 + y);
						tile_h = read(patterns + t * 16 + y + 8);
						palette_translation(tile_l, tile_h, tile_data0, tile_data1);
					}
										
					word_t shift = ((7 - x) % 4) * 2;
					word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
					/// @todo Palette
					_screen[_line * ScreenWidth + x] = color * 64;
					++sprite_pixel;
				}
			}
			// VBlank at 241
			if(_line == 241) _ppu_status |= VBlank;
			else if(_line == 0) _ppu_status &= ~VBlank;
			_line = (_line + 1) % 260;
		}
	}
	
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
	
	bool		_ppu_addr_upper = true;
	addr_t		_ppu_addr = 0;				// Access by $2006
	
	word_t*		_mem = nullptr;				// Access by $2007
	word_t*		_oam = nullptr;				// Access by $2004
	
	color_t*	_screen;
};