#pragma once

class APU {
  public:
    using word_t = uint8_t;
    using addr_t = uint16_t;

    inline word_t read(addr_t addr) { return _registers[addr % 0x17]; }

    inline void write(addr_t addr, word_t value) { _registers[addr % 0x17] = addr; }

  private:
    word_t _registers[0x17];
};
