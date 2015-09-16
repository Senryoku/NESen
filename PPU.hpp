#pragma once

/**
 * NES Picture Processing Unit
 *
 *
**/ 
class PPU
{
public:
	using word_t = uint8_t;
	
	word_t registers[8];
};