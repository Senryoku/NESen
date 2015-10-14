#include <sstream>
#include <iomanip>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "NES.hpp"

int main(int argc, char* argv[])
{
	std::string path = "tests/Ice Climber (USA, Europe).nes";
	
	if(argc > 1)
		path = argv[1];
	
	NES nes;
	if(!nes.load(path))
	{
		std::cerr << "Error loading '" << path << "'. Exiting..." << std::endl;
		return 0;
	}
	
	// For nestest.nes, don't forget to set PC at C000.
	//nes.cpu.set_test_log("tests/nestest_addr.log");
	
	nes.reset();
	
	float screen_scale = 2.0f;
	
	size_t padding = 200;
	sf::RenderWindow window(sf::VideoMode(screen_scale * nes.ppu.ScreenWidth + padding + 600, 
											screen_scale * nes.ppu.ScreenHeight + padding), 
											"NESen");
	window.setVerticalSyncEnabled(false);
	
	sf::Texture	nes_screen;
	if(!nes_screen.create(nes.ppu.ScreenWidth, nes.ppu.ScreenHeight))
		std::cerr << "Error creating the screen texture!" << std::endl;
	sf::Sprite nes_screen_sprite;
	nes_screen_sprite.setTexture(nes_screen);
	nes_screen_sprite.setPosition(padding / 2, padding / 2);
	nes_screen_sprite.setScale(screen_scale, screen_scale);
	
	sf::Texture	nes_tilemap;
	if(!nes_tilemap.create(16 * 8, 2 * 16 * 8))
		std::cerr << "Error creating the vram texture!" << std::endl;
	sf::Sprite nes_tilemap_sprite;
	nes_tilemap_sprite.setTexture(nes_tilemap);
	nes_tilemap_sprite.setPosition(screen_scale * nes.ppu.ScreenWidth + padding, 
		padding / 2);
	nes_tilemap_sprite.setScale(screen_scale, screen_scale);
	// 2 * (256 tiles * (8 * 8) bits per tile)
	size_t tile_map_size = 2 * 256 * 8 * 8;
	PPU::color_t* tile_map = new PPU::color_t[tile_map_size];
	std::memset(tile_map, 0, tile_map_size);
	
	sf::Font font;
	if(!font.loadFromFile("data/Hack-Regular.ttf"))
		std::cerr << "Error loading the font!" << std::endl;
	
	sf::Text debug_text;
	debug_text.setFont(font);
	debug_text.setCharacterSize(16);
	debug_text.setPosition(5, 0);
	
	sf::Text log_text;
	log_text.setFont(font);
	log_text.setCharacterSize(16);
	log_text.setPosition(5, 200 / 2 + 2 * nes.ppu.ScreenHeight);
	log_text.setString("Log");
	
	//bool first_loop = true;
	while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				switch(event.key.code)
				{
					case sf::Keyboard::Escape: window.close(); break;
					default: break;
				}
			}
		}
		
		do
		{
			nes.step();
		} while(!nes.ppu.completed_frame);
		
		nes_screen.update(reinterpret_cast<const uint8_t*>(nes.ppu.get_screen()));
		
		// Debug display Tilemap
		{
			PPU::word_t tile_l;
			PPU::word_t tile_h;
			PPU::word_t tile_data0, tile_data1;
			for(int t = 0; t < 512; ++t)
			{
				size_t tile_off = 8 * (t % 16) + (16 * 8 * 8) * (t / 16); 
				for(int y = 0; y < 8; ++y)
				{
					tile_l = nes.ppu.read(t * 16 + y);
					tile_h = nes.ppu.read(t * 16 + y + 8);
					PPU::palette_translation(tile_l, tile_h, tile_data0, tile_data1);
					for(int x = 0; x < 8; ++x)
					{
						PPU::word_t shift = ((7 - x) % 4) * 2;
						PPU::word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
						/// @Todo: Palettes
						tile_map[tile_off + 16 * 8 * y + x] = (4 - color) * (255/4.0);
					}
				}
			}
			nes_tilemap.update(reinterpret_cast<uint8_t*>(tile_map));
		}
		
		if(true)
		{
			std::stringstream dt;
			dt << "PC: " << Hexa(nes.cpu.get_pc());
			dt << " SP: " << Hexa(nes.cpu.get_sp());
			dt << " | OP: " << Hexa8(nes.cpu.get_next_opcode()) << " ";
			dt << Hexa8(nes.cpu.get_next_operand0()) << " ";
			dt << Hexa8(nes.cpu.get_next_operand1());
			dt << std::endl;
			dt << "X: " << Hexa8(nes.cpu.get_x());
			dt << " Y: " << Hexa8(nes.cpu.get_y());
			dt << " SP: " << Hexa8(nes.cpu.get_sp());
			dt << " PS: " << Hexa8(nes.cpu.get_ps());
			if(nes.cpu.check(CPU::StateMask::Carry)) dt << " C";
			if(nes.cpu.check(CPU::StateMask::Zero)) dt << " Z";
			if(nes.cpu.check(CPU::StateMask::Interrupt)) dt << " I";
			if(nes.cpu.check(CPU::StateMask::Decimal)) dt << " D";
			if(nes.cpu.check(CPU::StateMask::Break)) dt << " B";
			if(nes.cpu.check(CPU::StateMask::Overflow)) dt << " O";
			if(nes.cpu.check(CPU::StateMask::Negative)) dt << " N";
			dt << std::endl;
			/*
			dt << "LY: " << Hexa8(gpu.getLine());
			dt << " LCDC: " << Hexa8(gpu.getLCDControl());
			dt << " STAT: " << Hexa8(gpu.getLCDStatus());
			dt << " P1: " << Hexa8(mmu.read(MMU::P1));
			*/
			debug_text.setString(dt.str());
		}
		
	    window.clear(sf::Color::Black);
		window.draw(nes_screen_sprite);
		window.draw(nes_tilemap_sprite);
		window.draw(debug_text);
		window.draw(log_text);
        window.display();
	}
	
	delete[] tile_map;
}
