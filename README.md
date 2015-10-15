# NESen

WIP NES Emulator

## Todo List
* Use Qt ? (Should we remove SFML? It might be usefull for sound if not for rendering...)
* Controls
* CPU
  * Page crossing timing (+1 if page boundery is crossed)
  * Branch instructions timing (if branch, +1 on same page, +2 on different page)
  * (Unofficial opcodes timing)
  * (Some unofficial opcodes)
* PPU
  * Nametable mapping/mirroring (via cartridge?)
  * Attributes (Color Palettes)
  * Sprites
  
## Dependencies
* SFML 2.X (http://www.sfml-dev.org/) for graphical output and input handling.