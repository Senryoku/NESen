#include "PPU.hpp"

PPU::PPU() : _mem(new word_t[MemSize]), _oam(new word_t[OAMSize]), _screen(new color_t[240 * 256]) {
    reset();
}

PPU::~PPU() {
    delete[] _screen;
    delete[] _oam;
    delete[] _mem;
}

bool PPU::load_palette(const std::string& path) {
    std::ifstream pal_file(path, std::ios::binary);
    if(!pal_file) {
        std::cerr << "Error: Couldn't load palette at '" << path << "'." << std::endl;
        return false;
    } else {
        word_t r, b, g;
        size_t c = 0;
        while(pal_file) {
            pal_file >> r;
            pal_file >> g;
            pal_file >> b;
            rgb_palette[c] = color_t(r, g, b);
            ++c;
        }
        return true;
    }
}

void PPU::reset() {
    std::memset(_screen, 0, 240 * 256);
    std::memset(_mem, 0, MemSize);
}

void PPU::step(size_t cpu_cycles) {
    completed_frame = false;
    _cycles += 3 * cpu_cycles;
    if(_cycles > CyclesPerScanline) {
        _cycles -= CyclesPerScanline;
        if(_line < ScreenHeight) {
            // Background
            word_t t;
            word_t tile_l;
            word_t tile_h;
            word_t tile_data0 = 0, tile_data1 = 0;
            word_t sy = (_scroll_y + _line) & 7;
            word_t coarse_y = (_scroll_y + _line) >> 3;
            word_t bg_tile_pixel = _scroll_x & 7;
            addr_t nametable = 0x2000 + 0x400 * (_ppu_control & NameTableAddress);
            addr_t patterns = (_ppu_control & BackgoundPatternTableAddress) ? 0x1000 : 0;

            // Vertical Wrap
            if(coarse_y >= 30) {
                if(nametable < 0x2800)
                    nametable += 0x800;
                else
                    nametable -= 0x800;
                coarse_y -= 30;
            }

            addr_t attribute_table = nametable + 0x03C0;
            attribute_table += 8 * ((_scroll_y + _line) >> 5);

            nametable += 32 * coarse_y;

            word_t attribute = 0;
            bool   background_transparency[ScreenWidth];

            color_t universal_background_color = rgb_palette[_mem[0x3F00] & 0x3F];
            color_t color_cache[4][3]; // 4 palettes of 3 colors each
            // 0x3F00 : Universal background color
            // 0x3F01 : Background palette 0 Color 0
            // 0x3F02 : Background palette 0 Color 1
            // 0x3F03 : Background palette 0 Color 2
            // 0x3F04 : --
            // 0x3F05 : Background palette 1 Color 0
            // 0x3F06 : Background palette 1 Color 1
            // 0x3F07 : Background palette 1 Color 2
            // 0x3F08 : --
            // ...
            for(int i = 0; i < 4; ++i)
                for(int j = 0; j < 3; ++j) {
                    word_t val = _mem[0x3F01 + 4 * i + j];
                    color_cache[i][j] = rgb_palette[val & 0x3F];
                }

            bool top = ((_scroll_y + _line) % 32) < 16;
            for(size_t x = 0; x < ScreenWidth; ++x) {
                // Fetch Tile Data (When crossing tile border, or a the beginning of the scanline)
                if(bg_tile_pixel == 8 || x == 0) {
                    addr_t tile_x = (x + _scroll_x) >> 3;
                    if(tile_x >= 32) { // Next nametable (wraping)
                        if((nametable >= 0x2400 && nametable < 0x2800) || (nametable >= 0x2C00 && nametable < 0x3000))
                            t = _mem[nametable - 0x400 + tile_x - 32];
                        else
                            t = _mem[nametable + 0x400 + tile_x - 32];
                    } else
                        t = _mem[nametable + tile_x];
                    bg_tile_pixel = bg_tile_pixel % 8;
                    tile_l = read(patterns + t * 16 + sy);
                    tile_h = read(patterns + t * 16 + sy + 8);
                    tile_translation(tile_l, tile_h, tile_data0, tile_data1);

                    // Get palette from attribute table
                    addr_t attribute_x = ((x + _scroll_x) >> 5);
                    if(attribute_x >= 8) { // Next nametable (wraping)
                        attribute_x -= 8;
                        if((attribute_table >= 0x2400 && attribute_table < 0x2800) || (attribute_table >= 0x2C00 && attribute_table < 0x3000))
                            attribute = _mem[attribute_table - 0x400 + attribute_x];
                        else
                            attribute = _mem[attribute_table + 0x400 + attribute_x];
                    } else
                        attribute = _mem[attribute_table + attribute_x];
                    bool left = ((x + _scroll_x) % 32) < 16; // Each byte contains the palette index for 4 blocks of 2x2 tiles
                    if(top && !left)
                        attribute = attribute >> 2;
                    else if(!top && left)
                        attribute = attribute >> 4;
                    else if(!top && !left)
                        attribute = attribute >> 6;
                    attribute &= 3;
                }

                word_t shift = ((7 - bg_tile_pixel) % 4) * 2;
                word_t color = ((bg_tile_pixel > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
                if(color > 0) {
                    background_transparency[x] = false;
                    _screen[_line * ScreenWidth + x] = color_cache[attribute][color - 1];
                } else {
                    background_transparency[x] = true;
                    _screen[_line * ScreenWidth + x] = universal_background_color;
                }
                ++bg_tile_pixel;
            }

            // Sprites
            word_t              size = (_ppu_control & SpriteSize) ? 16 : 8;
            std::vector<size_t> sprites;
            sprites.reserve(8);
            for(size_t i = 0; i < OAMSize; i += 4) {
                word_t y = _oam[i];
                if(y <= _line && y + size > _line)
                    sprites.push_back(i);
                if(sprites.size() >= 8)
                    break;
            }

            for(int i = 0; i < 4; ++i)
                for(int j = 0; j < 3; ++j) {
                    word_t val = _mem[0x3F11 + 4 * i + j];
                    color_cache[i][j] = rgb_palette[val & 0x3F];
                }

            // Reverse order?
            for(size_t s : sprites) {
                word_t x = _oam[s + 3];
                word_t y = _oam[s] - _line;
                t = _oam[s + 1];
                attribute = _oam[s + 2];

                if(!(attribute & FlipY))
                    y = 8 - y;

                if(_ppu_control & SpriteSize) {
                    patterns = (t & 1) ? 0x1000 : 0;
                    t = t & 0xFE;
                    if(y > 7)
                        t++;
                } else {
                    patterns = (_ppu_control & SpritePatternTableAddress) ? 0x1000 : 0;
                }

                tile_l = read(patterns + t * 16 + (y & 7));
                tile_h = read(patterns + t * 16 + (y & 7) + 8);
                tile_translation(tile_l, tile_h, tile_data0, tile_data1);

                for(size_t p = 0; p < 8; ++p) {
                    word_t c_x = (attribute & FlipX) ? (7 - p) : p;
                    word_t shift = ((7 - c_x) % 4) * 2;
                    word_t color = ((c_x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;

                    if(s == 0 && color > 0 && !background_transparency[x + p])
                        _ppu_status |= Sprite0Hit;

                    if(color > 0 && (!(attribute & Priority) || background_transparency[x + p])) {
                        _screen[_line * ScreenWidth + x + p] = color_cache[attribute & Palette][color - 1];
                    }
                }
            }
        }

        _line = (_line + 1) % 262;
        // VBlank at 241 (to 260)
        if(_line == 241) {
            // if(_ppu_control & VBlankEnable) // Mmh ?
            _ppu_status |= VBlank;
        } else if(_line == 261) {       // Pre-render scanline (sometimes numbered -1)
            _ppu_status &= ~Sprite0Hit; // Clear Sprite0Hit Bit
        } else if(_line == 0) {
            completed_frame = true;
            _ppu_status &= ~VBlank;
        }

        _nmi = (_ppu_control & VBlankEnable) && (_ppu_status & VBlank);
    }
}