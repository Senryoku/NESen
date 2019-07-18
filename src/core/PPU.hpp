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
	static constexpr size_t ScreenWidth = 256;
	static constexpr size_t ScreenHeight = 240;
	static constexpr size_t CyclesPerScanline = 240;

	static constexpr size_t MemSize = 0x4000;
	static constexpr size_t OAMSize = 0x100;
	
	enum PPUControl : word_t
	{
		NameTableAddress             = 0x03,
		VerticalWrite                = 0x04,
		SpritePatternTableAddress    = 0x08,
		BackgoundPatternTableAddress = 0x10,
		SpriteSize                   = 0x20,
		PPUMasterSlaveMode           = 0x40,
		VBlankEnable                 = 0x80
	};
	
	enum PPUMask : word_t
	{
		Grayscale          = 0x01,
		LeftBackgroundMask = 0x02,
		LeftSpriteMask     = 0x04,
		BackgroundMask     = 0x08,
		SpriteMask         = 0x10,
		EmphasizeRed       = 0x20,
		EmphasizeGreen     = 0x40,
		EmphasizeBlue      = 0x80
	};
	
	enum PPUStatus : word_t
	{
		SpriteOverflow = 0x20,
		Sprite0Hit     = 0x40,
		VBlank         = 0x80
	};
	
	enum SpriteAttribute
	{
		Palette		= 0x03,
		Priority 	= 0x20,
		FlipX	 	= 0x40,
		FlipY	 	= 0x80
	};
	
	Cartridge* cartridge = nullptr;
	
	bool completed_frame = false;
	inline const color_t* get_screen() const { return _screen; }
	inline word_t get_mem(addr_t addr) const { return _mem[addr]; }
	
	PPU();
	~PPU();
	bool load_palette(const std::string& path);
	void reset();
	void step(size_t cpu_cycles);
	
	/// Access from CPU
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
				static word_t internal_buffer = 0;
				word_t r;
				if(_ppu_addr < 0x3F00)
				{
					r = internal_buffer;
					internal_buffer = _mem[_ppu_addr];
				} else {
					r = _mem[_ppu_addr];
					internal_buffer = _mem[_ppu_addr - 0x1F00];
				}
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
			case 0x06: // PPU read/write address (two writes: most significant byte, least significant byte)
				if(_ppu_addr_upper)
					_ppu_addr = (_ppu_addr & 0x00FF) | (value << 8);
				else
					_ppu_addr = (_ppu_addr & 0xFF00) | value;
				_ppu_addr_upper = !_ppu_addr_upper;
			break;
			case 0x07: // PPU data read/write
				_mem[_ppu_addr] = value;
				/// Nametables range - Mirroring
				if(_ppu_addr >= 0x2000 && _ppu_addr <= 0x2EFF) {
					if(cartridge->get_mirroring() == Cartridge::Horizontal) {
						if(_ppu_addr < 0x2400 || (_ppu_addr >= 0x2800 && _ppu_addr < 0x2C00))
							_mem[_ppu_addr + 0x400] = value;
						else
							_mem[_ppu_addr - 0x400] = value;
					} else if(cartridge->get_mirroring() == Cartridge::Vertical) {
						if(_ppu_addr < 0x2800)
							_mem[_ppu_addr + 0x800] = value;
						else
							_mem[_ppu_addr - 0x800] = value;
					}
				}
				
				// 0x3000 - 0x3EFF Mirrors 0x2000 - 0x2EFF
				if(_ppu_addr >= 0x3000 && _ppu_addr <= 0x3EFF)
					_mem[_ppu_addr - 0x1000] = value;
				
				// Palette Mirroring
				if(_ppu_addr == 0x3F00 || _ppu_addr == 0x3F04 || _ppu_addr == 0x3F08 ||  _ppu_addr == 0x3F0C)
					_mem[_ppu_addr + 0x10] = value;
				if(_ppu_addr == 0x3F10 || _ppu_addr == 0x3F14 || _ppu_addr == 0x3F18 ||  _ppu_addr == 0x3F1C)
					_mem[_ppu_addr - 0x10] = value;
				
				// 0x3F20 - 0x3FFF Mirrors 0x3F00 - 0x3F1F
				if(_ppu_addr >= 0x3F20 && _ppu_addr <= 0x3FFF)
					_mem[0x3F00 + ((_ppu_addr - 0x3F20) % 0x20)] = value;
				
				_ppu_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
				//std::cout << "PPU Write: " << Hexa(_ppu_addr) << " = " << Hexa8(value) << std::endl;
			break;
			default:
				std::cerr << "Write on unsupported PPU address: " << Hexa(addr) << std::endl;
			break;
		}
	}
	
	inline bool check_nmi()
	{
		bool r = _nmi;
		_nmi = false;
		return r;
	}
/*
	color_t rgb_palette[0x40] = {
		color_t(0x46, 0x46, 0x46), color_t(0x00, 0x01, 0x54), color_t(0x00, 0x00, 0x70), color_t(0x07, 0x00, 0x6b), color_t(0x28, 0x00, 0x48), color_t(0x3c, 0x00, 0x0e), color_t(0x3e, 0x00, 0x00), color_t(0x2c, 0x00, 0x00), color_t(0x0d, 0x03, 0x00), color_t(0x00, 0x15, 0x00), color_t(0x00, 0x1f, 0x00), color_t(0x00, 0x1f, 0x00), color_t(0x00, 0x14, 0x20), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00), 
		color_t(0x9d, 0x9d, 0x9d), color_t(0x00, 0x41, 0xb0), color_t(0x18, 0x25, 0xd5), color_t(0x4a, 0x0d, 0xcf), color_t(0x75, 0x00, 0x9f), color_t(0x90, 0x01, 0x53), color_t(0x92, 0x0f, 0x00), color_t(0x7b, 0x28, 0x00), color_t(0x51, 0x44, 0x00), color_t(0x20, 0x5c, 0x00), color_t(0x00, 0x69, 0x00), color_t(0x00, 0x69, 0x16),	color_t(0x00, 0x5a, 0x6a), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00), 
		color_t(0xfe, 0xff, 0xff), color_t(0x48, 0x96, 0xff), color_t(0x62, 0x6d, 0xff), color_t(0x8e, 0x5b, 0xff), color_t(0xd4, 0x5e, 0xff), color_t(0xf1, 0x60, 0xb4), color_t(0xf3, 0x6f, 0x5e), color_t(0xdc, 0x88, 0x17), color_t(0xb2, 0xa4, 0x00), color_t(0x7f, 0xbd, 0x00), color_t(0x53, 0xca, 0x28), color_t(0x38, 0xca, 0x76), color_t(0x36, 0xbb, 0xcb), color_t(0x2b, 0x2b, 0x2b), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00), 
		color_t(0xfe, 0xff, 0xff), color_t(0xb0, 0xd2, 0xff), color_t(0xb6, 0xbb, 0xff), color_t(0xcb, 0xb4, 0xff), color_t(0xed, 0xbc, 0xff), color_t(0xf9, 0xbd, 0xe0), color_t(0xfa, 0xc3, 0xbd), color_t(0xf0, 0xce, 0x9f),	color_t(0xdf, 0xd9, 0x90), color_t(0xca, 0xe3, 0x93), color_t(0xb8, 0xe9, 0xa6), color_t(0xad, 0xe9, 0xc6), color_t(0xac, 0xe3, 0xe9), color_t(0xa7, 0xa7, 0xa7), color_t(0x00, 0x00, 0x00), color_t(0x00, 0x00, 0x00)            
	};
*/
	color_t rgb_palette[0x40] = {
		color_t( 84,  84,  84),  color_t(  0,  30, 116),  color_t(  8,  16, 144),  color_t( 48,   0, 136),  color_t( 68,   0, 100),  color_t( 92,   0,  48),  color_t( 84,   4,   0),  color_t( 60,  24,   0),  color_t( 32,  42,   0),  color_t(  8,  58,   0),  color_t(  0,  64,   0),  color_t(  0,  60,   0),  color_t(  0,  50,  60),  color_t(  0,   0,   0),  color_t(  0,   0,   0),  color_t(  0,   0,   0),
		color_t(152, 150, 152),  color_t(  8,  76, 196),  color_t( 48,  50, 236),  color_t( 92,  30, 228),  color_t(136,  20, 176),  color_t(160,  20, 100),  color_t(152,  34,  32),  color_t(120,  60,   0),  color_t( 84,  90,   0),  color_t( 40, 114,   0),  color_t(  8, 124,   0),  color_t(  0, 118,  40),  color_t(  0, 102, 120),  color_t(  0,   0,   0),  color_t(  0,   0,   0),  color_t(  0,   0,   0),
		color_t(236, 238, 236),  color_t( 76, 154, 236),  color_t(120, 124, 236),  color_t(176,  98, 236),  color_t(228,  84, 236),  color_t(236,  88, 180),  color_t(236, 106, 100),  color_t(212, 136,  32),  color_t(160, 170,   0),  color_t(116, 196,   0),  color_t( 76, 208,  32),  color_t( 56, 204, 108),  color_t( 56, 180, 204),  color_t( 60,  60,  60),  color_t(  0,   0,   0),  color_t(  0,   0,   0),
		color_t(236, 238, 236),  color_t(168, 204, 236),  color_t(188, 188, 236),  color_t(212, 178, 236),  color_t(236, 174, 236),  color_t(236, 174, 212),  color_t(236, 180, 176),  color_t(228, 196, 144),  color_t(204, 210, 120),  color_t(180, 222, 120),  color_t(168, 226, 144),  color_t(152, 226, 180),  color_t(160, 214, 228),  color_t(160, 162, 160),  color_t(  0,   0,   0),  color_t(  0,   0,   0)
	};

	static inline void tile_translation(word_t l, word_t h, word_t& r0, word_t& r1)
	{
		r0 = r1 = 0;
		for(int i = 0; i < 4; i++)
			r0 |= (((l & (1 << (i + 4))) >> (4 + i)) << (2 * i)) | 
					(((h & (1 << (i + 4))) >> (4 + i))  << (2 * i + 1));
		for(int i = 0; i < 4; i++)
			r1 |= ((l & (1 << i)) << (2 * i - i)) | 
					((h & (1 << i)) << (2 * i + 1 - i));
	}

	// Debug accessors
	inline word_t get_control_reg() const { return _ppu_control; }
	inline word_t get_mask_reg() const { return _ppu_mask; }
	inline word_t get_status_reg() const { return _ppu_status; }
	inline word_t get_oam_addr_reg() const { return _oam_addr; }
	inline word_t get_scroll_x() const { return _scroll_x; }
	inline word_t get_scroll_y() const { return _scroll_y; }
	
	inline color_t get_color(word_t palette_idx, word_t color_idx) const {
		if(color_idx == 0) {
			return rgb_palette[_mem[0x3F00] & 0x3F]; 
		} else {
			return rgb_palette[_mem[0x3F01 + 4 * palette_idx + color_idx - 1] & 0x3F]; 
		}
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
	
	bool 		_nmi = false; 				// Non-Maskable Interrupt 
	
	bool		_ppu_addr_upper = true;
	addr_t		_ppu_addr = 0;				// Access by $2006
	
	word_t*		_mem = nullptr;				// Access by $2007
	word_t*		_oam = nullptr;				// Access by $2004
	
	color_t*	_screen;
};
