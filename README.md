# NESen

WIP NES Emulator

## Todo List
* GUI with wxWidget
* ~~Controls~~
* Complete memory mapping (and cartridge)
* CPU
  * IRQ
  * Page crossing timing (+1 if page boundery is crossed)
  * Branch instructions timing (if branch, +1 on same page, +2 on different page)
  * (Unofficial opcodes timing)
  * (Some unofficial opcodes)
* PPU
  * Nametable mapping/mirroring (via cartridge?)
  * ~~Attributes (Color Palettes)~~
  * ~~Sprites~~
  
## Dependencies
* SFML 2.X (http://www.sfml-dev.org/) for graphical output and input handling.