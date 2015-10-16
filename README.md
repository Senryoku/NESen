# NESen

Unfinished NES Emulator.

## Status

Core CPU Emulation is pretty much done, everything else are quick hacks. 
It runs Donkey Kong at an awkward speed, but that's about it. Next steps should be debugging (What's the matter with emulation speed? Why is Ice Climbers not working - at all?) and mappers implementation, but motivation is eluding me.

## Todo List
* Debug (Speed; Crashing mapper 000 games)
* ~~Controls~~
* Complete memory mapping (and cartridge)
* CPU
  * ~~IRQ~~
  * Page crossing timing (+1 if page boundery is crossed)
  * Branch instructions timing (if branch, +1 on same page, +2 on different page)
  * (Unofficial opcodes timing)
  * (Some unofficial opcodes)
* PPU
  * Nametable mapping/mirroring (via cartridge?)
  * ~~Attributes (Color Palettes)~~
  * ~~Sprites~~
* (GUI with wxWidget)
  
## Dependencies
* SFML 2.X (http://www.sfml-dev.org/) for graphical output and input handling.
