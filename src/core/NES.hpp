#pragma once

#include "CPU.hpp"

class NES {
  public:
    CPU       cpu;
    APU       apu;
    PPU       ppu;
    Cartridge cartridge;

    NES(size_t RAMSize = 0x800) : cpu(RAMSize) { init(); }

    NES(const std::string& path) {
        init();
        load(path);
    }

    ~NES() {}

    void init() {
        cpu.ppu = &ppu;
        cpu.cartridge = &cartridge;
        cpu.apu = &apu;
        ppu.cartridge = &cartridge;
    }

    bool load(const std::string& path) { return cartridge.load(path); }

    void reset() {
        cpu.reset();
        ppu.reset();
    }

    void run() {
        reset();
        while(!_shutdown) {
            step();
        }
    }

    void step() {
        cpu.step();
        ppu.step(cpu.get_cycles());
    }

  private:
    bool  _shutdown = false;
    float _ppucpuRatio = 3.0;
};
