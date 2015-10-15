#include "PPU.hpp"

void PPU::step(size_t cpu_cycles)
{
	completed_frame = false;
	_cycles += 3 * cpu_cycles;
	if(_cycles > CyclesPerScanline)
	{
		_cycles -= CyclesPerScanline;
		if(_line < ScreenHeight)
		{
			// Background
			word_t t;
			word_t tile_l;
			word_t tile_h;
			word_t tile_data0 = 0, tile_data1 = 0;
			word_t y = (_scroll_y + _line) & 7;
			word_t sprite_pixel = _scroll_x & 7;
			addr_t nametable = 0x2000 + 0x400 * (_ppu_control & NameTableAddress);
			addr_t attributetable = nametable + 0x03C0;
			addr_t patterns = (_ppu_control & BackgoundPatternTableAddress) ? 0x1000 : 0;
			nametable += 32 * ((_scroll_y + _line) >> 3);
			attributetable += 8 * ((_scroll_y + _line) >> 5);
			word_t attribute = 0;
			color_t	color_cache[4][3]; // 4 palettes of 3 colors each 
			bool background_transparency[ScreenWidth];
			for(int i = 0; i < 4; ++i)
				for(int j = 0; j < 3; ++j)
				{
					word_t val = _mem[0x3F01 + 4 * i + j];
					color_cache[i][j] = rgb_palette[val & 0x7F];
				}
			bool top = ((_scroll_y + _line) % 32 < 16);
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
					
					attribute = _mem[attributetable + ((x + _scroll_x) >> 5)];
					bool left = ((x + _scroll_x) % 32 < 16);
					if(top && !left)
						attribute = attribute >> 2;
					else if(!top && left)
						attribute = attribute >> 4;
					else if(!top && !left)
						attribute = attribute >> 6;
					attribute &= 3;
				}

				word_t shift = ((7 - sprite_pixel) % 4) * 2;
				word_t color = ((sprite_pixel > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
				if(color > 0)
				{
					background_transparency[x] = false;
					_screen[_line * ScreenWidth + x] = color_cache[attribute][color - 1];
				} else {
					background_transparency[x] = true;
					_screen[_line * ScreenWidth + x] = 0;
				}
				++sprite_pixel;
			}
		
			// Sprites
			word_t size = (_ppu_control & SpriteSize) ? 16 : 8;
			std::vector<size_t> sprites;
			sprites.reserve(8);
			for(size_t i = 0; i < OAMSize; i += 4)
			{
				word_t y = _oam[i];
				if(y <= _line && y + size > _line)
					sprites.push_back(i);
				if(sprites.size() >= 8) break;
			}
			
			for(int i = 0; i < 4; ++i)
				for(int j = 0; j < 3; ++j)
				{
					word_t val = _mem[0x3F11 + 4 * i + j];
					color_cache[i][j] = rgb_palette[val & 0x7F];
				}
				
			// Reverse order?
			for(size_t s : sprites)
			{
				word_t x = _oam[s + 3];
				word_t y = _oam[s] - _line;
				t = _oam[s + 1];
				attribute = _oam[s + 2];
				
				if(!(attribute & FlipY)) y = 8 - y;
				
				if(_ppu_control & SpriteSize)
				{
					patterns = (t & 1) ? 0x1000 : 0;
					t = t & 0xFE;
					if(y > 7) t++;
				} else {
					patterns = (_ppu_control & SpritePatternTableAddress) ? 0x1000 : 0;
				}

				tile_l = read(patterns + t * 16 + (y & 7));
				tile_h = read(patterns + t * 16 + (y & 7) + 8);
				palette_translation(tile_l, tile_h, tile_data0, tile_data1);
				
				for(size_t p = 0; p < 8; ++p)
				{
					word_t c_x = (attribute & FlipX) ? (7 - p) : p;
					word_t shift = ((7 - c_x) % 4) * 2;
					word_t color = ((c_x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
					if(color > 0 && (!(attribute & Priority) || background_transparency[x + p]))
					{
						_screen[_line * ScreenWidth + x + p] = color_cache[attribute & Palette][color - 1];
					}
				}
			}
		}

		_line = (_line + 1) % 261;
		// VBlank at 241
		if(_line == 241)
		{
			_ppu_status |= VBlank;
		} else if(_line == 0) {
			completed_frame = true;
			_ppu_status &= ~VBlank;
		}
		
		_nmi = (_ppu_control & VBlankEnable) && (_ppu_status & VBlank);
	}
}