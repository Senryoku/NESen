#include <sstream>
#include <iomanip>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "NES.hpp"
#include "Common.hpp"
#include "CommandLine.hpp"

bool debug = false;
bool step = true;
bool real_speed = true;

// Timing
sf::Clock timing_clock;
double frame_time = 0;
size_t speed_update = 10;
double speed = 100;
uint64_t elapsed_cycles = 0;
uint64_t speed_mesure_cycles = 0;
size_t frame_count = 0;

// Debug Display
addr_t nametable_addr = 0x2000;
bool background_pattern = false;

int main(int argc, char* argv[])
{
	config::set_folder(argv[0]);
	
	NES nes;
	std::string path = config::to_abs("tests/Ice Climber (USA, Europe).nes");
	
	bool regression_test = has_option(argc, argv, "-r");
	if(regression_test)
	{
		debug = false;
		path = config::to_abs("tests/nestest.nes");
		nes.cpu.set_test_log(config::to_abs("tests/nestest_addr.log"));
	} else if(get_file(argc, argv)) {
		path = get_file(argc, argv);
	}
	
	if(!nes.load(path))
	{
		std::cerr << "Error loading '" << path << "'. Exiting..." << std::endl;
		return 0;
	}
	
	nes.reset();
	if(regression_test)
		nes.cpu.set_pc(0xC000);
		
	float screen_scale = 2.0f;
	
	nes.cpu.controller_callbacks[0] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 0)) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::A); };
	nes.cpu.controller_callbacks[1] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 1)) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Z); };
	nes.cpu.controller_callbacks[2] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 6)) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Q); };
	nes.cpu.controller_callbacks[3] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 7)) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::S); };
	nes.cpu.controller_callbacks[4] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::Y) < -50) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Up); };
	nes.cpu.controller_callbacks[5] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::Y) > 50) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Down); };
	nes.cpu.controller_callbacks[6] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::X) < -50) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Left); };
	nes.cpu.controller_callbacks[7] = [&] () -> bool { for(int i = 0; i < sf::Joystick::Count; ++i) if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::X) > 50) return true; return sf::Keyboard::isKeyPressed(sf::Keyboard::Right); };
	
	size_t padding = 200;
	sf::RenderWindow window(sf::VideoMode(screen_scale * nes.ppu.ScreenWidth + 2 * padding + screen_scale * (16 * 8 + 32 * 8), 
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
	color_t* tile_map = new color_t[tile_map_size];
	std::memset(tile_map, 0, tile_map_size);
	
	sf::Texture	nes_nametable;
	if(!nes_nametable.create(32 * 8, 30 * 8))
		std::cerr << "Error creating the vram texture!" << std::endl;
	sf::Sprite nes_nametable_sprite;
	nes_nametable_sprite.setTexture(nes_nametable);
	nes_nametable_sprite.setPosition(nes_tilemap_sprite.getPosition().x + 
		nes_tilemap_sprite.getGlobalBounds().width + padding / 2, 
		padding / 2);
	nes_nametable_sprite.setScale(screen_scale, screen_scale);
	size_t nametable_size = 30 * 32 * 8 * 8;
	color_t* nametable = new color_t[nametable_size];
	std::memset(nametable, 0, nametable_size);
	
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
					case sf::Keyboard::Return: debug = !debug; break;
					case sf::Keyboard::Space: step = true; break;
					case sf::Keyboard::B: background_pattern = !background_pattern; break; 
					case sf::Keyboard::N: 
						nametable_addr = 0x2000 + ((nametable_addr + 0x400) % 0x1000); 
						std::cout << "Nametable: " << Hexa(nametable_addr) << std::endl; 
						break;
					default: break;
				}
			}
		}
		
		double nes_time = double(elapsed_cycles) / nes.cpu.ClockRate;
		double diff = nes_time - timing_clock.getElapsedTime().asSeconds();
		if(real_speed && !debug && diff > 0)
			sf::sleep(sf::seconds(diff));
		if(!debug || step)
		{
			step = false;
			do
			{
				nes.step();
				elapsed_cycles += nes.cpu.get_cycles();
				speed_mesure_cycles += nes.cpu.get_cycles();
			} while(!debug && !nes.ppu.completed_frame);
		
			// Update screen
		
			nes_screen.update(reinterpret_cast<const uint8_t*>(nes.ppu.get_screen()));
			
			// Debug display Tilemap
			{
				word_t tile_l;
				word_t tile_h;
				word_t tile_data0, tile_data1;
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
							word_t shift = ((7 - x) % 4) * 2;
							word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
							/// @Todo: Palettes
							tile_map[tile_off + 16 * 8 * y + x] = color * 64;
						}
					}
				}
				nes_tilemap.update(reinterpret_cast<uint8_t*>(tile_map));
			}
			
			// Nametables
			{
				word_t tile_l;
				word_t tile_h;
				word_t tile_data0, tile_data1;
				for(int n = 0; n < 32 * 30; ++n)
				{
					size_t tile_off = 8 * (n % 32) + (32 * 8 * 8) * (n / 32); 
					word_t t = nes.ppu.get_mem(nametable_addr + n);
					//std::cout << " n: " << Hexa8(n) << " t: " << Hexa8(t) << std::endl;
					for(int y = 0; y < 8; ++y)
					{
						tile_l = nes.ppu.read(0x1000 * background_pattern + t * 16 + y);
						tile_h = nes.ppu.read(0x1000 * background_pattern + t * 16 + y + 8);
						PPU::palette_translation(tile_l, tile_h, tile_data0, tile_data1);
						for(int x = 0; x < 8; ++x)
						{
							word_t shift = ((7 - x) % 4) * 2;
							word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
							/// @Todo: Palettes
							nametable[tile_off + 32 * 8 * y + x] = color * 64;
						}
					}
				}
				nes_nametable.update(reinterpret_cast<uint8_t*>(nametable));
			}
			
			if(--speed_update == 0)
			{
				speed_update = 10;
				double t = timing_clock.getElapsedTime().asSeconds();
				speed = 100.0 * (double(speed_mesure_cycles) / nes.cpu.ClockRate) / (t - frame_time);
				frame_time = t;
				speed_mesure_cycles = 0;
				
				std::stringstream dt;
				dt << "Speed " << std::dec << std::fixed << std::setw(4) << std::setprecision(1) << speed << "%";
				window.setTitle(std::string("NESen - ").append(dt.str()));
				debug_text.setString(dt.str());
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
				dt << "A: " << Hexa8(nes.cpu.get_acc());
				dt << " X: " << Hexa8(nes.cpu.get_x());
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
		}
		
	    window.clear(sf::Color::Black);
		window.draw(nes_screen_sprite);
		window.draw(nes_tilemap_sprite);
		window.draw(nes_nametable_sprite);
		window.draw(debug_text);
		window.draw(log_text);
        window.display();
	}
	
	delete[] tile_map;
}
