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

static bool background_transparency[PPU::ScreenWidth];

void PPU::draw_line_sprites() {
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

    color_t universal_background_color = rgb_palette[_mem[0x3F00] & 0x3F];
    color_t color_cache[4][3]; // 4 palettes of 3 colors each
    for(int i = 0; i < 4; ++i)
        for(int j = 0; j < 3; ++j) {
            word_t val = _mem[0x3F11 + 4 * i + j];
            color_cache[i][j] = rgb_palette[val & 0x3F];
        }

    // Reverse order?
    for(const size_t& s : sprites) {
        word_t x = _oam[s + 3];
        word_t y = _oam[s] - _line;
        word_t t = _oam[s + 1];
        word_t attribute = _oam[s + 2];
        word_t patterns;

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

        word_t tile_data0, tile_data1;
        word_t tile_l = read(patterns + t * 16 + (y & 7));
        word_t tile_h = read(patterns + t * 16 + (y & 7) + 8);
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

void PPU::background_step() {
    static word_t attribute;
    static word_t tile_data0 = 0, tile_data1 = 0;

    auto coarse_x = (_v & 0x1f) * 8;

    auto bg_tile_pixel = (_cycles - 1 + coarse_x + _x) & 7;
    if(_cycles == 1 || bg_tile_pixel == 0) {
        auto   fine_y = (_v >> 12) & 7;
        addr_t tile_addr = 0x2000 | (_v & 0x0FFF);
        addr_t attribute_addr = 0x23C0 | (_v & 0x0C00) | ((_v >> 4) & 0x38) | ((_v >> 2) & 0x07);
        auto   tile = mem_read(tile_addr);
        addr_t patterns = (_ppu_control & BackgoundPatternTableAddress) ? 0x1000 : 0;
        auto   tile_l = mem_read(patterns + tile * 16 + fine_y);
        auto   tile_h = mem_read(patterns + tile * 16 + fine_y + 8);
        tile_translation(tile_l, tile_h, tile_data0, tile_data1);
        attribute = mem_read(attribute_addr);

        bool top = ((((_v >> 5) & 0x1f) * 8 + fine_y) % 32) < 16;
        bool left = ((coarse_x + _x) % 32) < 16; // Each byte contains the palette index for 4 blocks of 2x2 tiles
        auto shift = (top ? 0 : 4) + (left ? 0 : 2);
        attribute = attribute >> shift;
        attribute &= 3;

        if((_v & 0x001F) == 31) { // if coarse X == 31
            _v &= ~0x001F;        // coarse X = 0
            _v ^= 0x0400;         // switch horizontal nametable
        } else {                  //
            _v += 1;              // increment coarse X
        }
    }

    word_t shift = ((7 - bg_tile_pixel) & 3) << 1;
    word_t color = ((bg_tile_pixel > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
    if(color > 0) {
        background_transparency[_cycles - 1] = false;
        word_t val = mem_read(0x3F01 + 4 * attribute + (color - 1));
        _screen[_line * ScreenWidth + _cycles - 1] = rgb_palette[val & 0x3F];
    } else {
        background_transparency[_cycles - 1] = true;
        _screen[_line * ScreenWidth + _cycles - 1] = rgb_palette[_mem[0x3F00] & 0x3F];
    }
}

void PPU::step() {
    const auto bg_enabled = (_ppu_mask & BackgroundMask);
    const auto sprites_enabled = (_ppu_mask & SpriteMask);
    const auto rendering_enabled = bg_enabled || sprites_enabled;

    if(rendering_enabled && _line < ScreenHeight) {
        if(bg_enabled && _cycles > 0 && _cycles <= ScreenWidth) {
            background_step();
        }

        if(_cycles == 256) {
            if((_v & 0x7000) != 0x7000)         // if fine Y < 7
                _v += 0x1000;                   // increment fine Y
            else {                              //
                _v &= ~0x7000;                  // fine Y = 0
                addr_t y = (_v & 0x03E0) >> 5;  // let y = coarse Y
                if(y == 29) {                   //
                    y = 0;                      // coarse Y = 0
                    _v ^= 0x0800;               // switch vertical nametable
                } else if(y == 31)              //
                    y = 0;                      // coarse Y = 0, nametable not switched
                else                            //
                    y += 1;                     // increment coarse Y
                _v = (_v & ~0x03E0) | (y << 5); // put coarse Y back into v
            }
        } else if(_cycles == 257) {
            _v = (_v & 0b111101111100000) | (_t & 0b000010000011111);
        }
    }

    if(sprites_enabled && _line < ScreenHeight && _cycles == 257)
        draw_line_sprites(); // FIXME: Quick Hack, sprites are still rendered line by line.

    if(rendering_enabled && _line == 261 && (_cycles >= 280 && _cycles <= 304))
        _v = (_v & 0b000010000011111) | (_t & 0b111101111100000);

    ++_cycles;

    if(_cycles > 340) {
        _cycles = 0;
        _line = (_line + 1) % 262;
        // VBlank at 241 (to 260)
        if(_line == 241) {
            _ppu_status |= VBlank;
            _nmi = (_ppu_control & VBlankEnable) && (_ppu_status & VBlank);
        } else if(_line == 261) {       // Pre-render scanline (sometimes numbered -1)
            _ppu_status &= ~Sprite0Hit; // Clear Sprite0Hit Bit
            _v = (_v & 0x841F) | (_t & 0x7BE0);
        } else if(_line == 0) {
            completed_frame = true;
            _ppu_status &= ~VBlank;
        }
    }
}

void PPU::step(size_t cpu_cycles) {
    completed_frame = false;
    auto cycles = 3 * cpu_cycles;
    for(auto i = 0; i < cycles; ++i)
        step();
}