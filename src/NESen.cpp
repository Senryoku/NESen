#include <filesystem>
#include <iomanip>
#include <sstream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include <core/NES.hpp>
#include <tools/CommandLine.hpp>

bool debug = false;
bool step = true;
bool real_speed = true;
bool draw_gui = false;

// Timing
sf::Clock timing_clock;
double    frame_time = 0;
size_t    speed_update = 10;
double    speed = 100;
uint64_t  elapsed_cycles = 0;
uint64_t  speed_mesure_cycles = 0;
size_t    frame_count = 0;

// Debug Display
bool background_pattern = true;

const std::vector<std::string> single_roms{"01-basics.nes", "02-implied.nes", "03-immediate.nes", "04-zero_page.nes", "05-zp_xy.nes", "06-absolute.nes",
                                           "07-abs_xy.nes", "08-ind_x.nes",   "09-ind_y.nes",     "10-branches.nes",  "11-stack.nes", "12-jmp_jsr.nes",
                                           "13-rts.nes",    "14-rti.nes",     "15-brk.nes",       "16-special.nes"};

std::string explore(const std::string& path) {
    std::string r;

    // print all the files and directories within directory
    for(const auto& entry : std::filesystem::directory_iterator(path)) {
        if(entry.path().filename() == "." || entry.path().filename() == "..")
            continue;
        if(std::filesystem::is_directory(entry.path())) {
            if(ImGui::TreeNode(entry.path().string().c_str())) {
                r = explore(entry.path().string());
                ImGui::TreePop();
            }
        } else {
            if(ImGui::Button(entry.path().string().c_str())) {
                return entry.path().string();
            }
        }
    }

    return r;
}

int main(int argc, char* argv[]) {
    config::set_folder(argv[0]);

    NES         nes;
    std::string path = "tests/Super Mario Bros. (Europe) (Rev 0A).nes";

    if(get_file(argc, argv))
        path = get_file(argc, argv);

    if(!nes.load(path)) {
        Log::error("Error loading '", path, "'. Exiting...");
        return 0;
    }

    nes.reset();

    float screen_scale = 2.0f;

    nes.cpu.controller_callbacks[0] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 0))
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::A);
    };
    nes.cpu.controller_callbacks[1] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 1))
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
    };
    nes.cpu.controller_callbacks[2] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 6))
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
    };
    nes.cpu.controller_callbacks[3] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::isButtonPressed(i, 7))
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::S);
    };
    nes.cpu.controller_callbacks[4] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::Y) < -50)
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    };
    nes.cpu.controller_callbacks[5] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::Y) > 50)
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
    };
    nes.cpu.controller_callbacks[6] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::X) < -50)
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    };
    nes.cpu.controller_callbacks[7] = [&]() -> bool {
        for(int i = 0; i < sf::Joystick::Count; ++i)
            if(sf::Joystick::isConnected(i) && sf::Joystick::getAxisPosition(i, sf::Joystick::X) > 50)
                return true;
        return sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    };

    size_t           padding = 200;
    sf::RenderWindow window(sf::VideoMode(screen_scale * nes.ppu.ScreenWidth + padding, screen_scale * nes.ppu.ScreenHeight + padding), "NESen");
    window.setVerticalSyncEnabled(false);
    ImGui::SFML::Init(window);

    sf::Texture nes_screen;
    if(!nes_screen.create(nes.ppu.ScreenWidth, nes.ppu.ScreenHeight))
        std::cerr << "Error creating the screen texture!" << std::endl;
    sf::Sprite nes_screen_sprite;
    nes_screen_sprite.setTexture(nes_screen);
    nes_screen_sprite.setPosition(padding / 2, padding / 2);
    nes_screen_sprite.setScale(screen_scale, screen_scale);

    sf::Texture nes_tilemap;
    if(!nes_tilemap.create(16 * 8, 2 * 16 * 8))
        std::cerr << "Error creating the vram texture!" << std::endl;
    size_t   tile_map_size = 2 * 256 * 8 * 8;
    color_t* tile_map = new color_t[tile_map_size];
    std::memset(tile_map, 0, tile_map_size);

    sf::Texture nes_nametable[4];
    color_t*    nametable[4];
    size_t      nametable_size = 30 * 32 * 8 * 8;
    for(int i = 0; i < 4; ++i) {
        if(!nes_nametable[i].create(32 * 8, 30 * 8))
            std::cerr << "Error creating the vram texture!" << std::endl;
        nametable[i] = new color_t[nametable_size];
        std::memset(nametable[i], 0, nametable_size);
    }

    sf::Clock gui_delta_clock;
    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if(event.type == sf::Event::Closed)
                window.close();
            if(event.type == sf::Event::KeyPressed) {
                switch(event.key.code) {
                    case sf::Keyboard::Escape: draw_gui = !draw_gui; break;
                    case sf::Keyboard::Return: debug = !debug; break;
                    case sf::Keyboard::Space: step = true; break;
                    case sf::Keyboard::B: background_pattern = !background_pattern; break;
                    case sf::Keyboard::M: real_speed = !real_speed; break;
                    default: break;
                }
            }
        }

        double nes_time = double(elapsed_cycles) / nes.cpu.ClockRate;
        double diff = nes_time - timing_clock.getElapsedTime().asSeconds();
        if(real_speed && !debug && diff > 0)
            sf::sleep(sf::seconds(diff));
        if(!debug || step) {
            step = false;
            do {
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
                for(int t = 0; t < 512; ++t) {
                    size_t tile_off = 8 * (t % 16) + (16 * 8 * 8) * (t / 16);
                    for(int y = 0; y < 8; ++y) {
                        tile_l = nes.ppu.read(t * 16 + y);
                        tile_h = nes.ppu.read(t * 16 + y + 8);
                        PPU::tile_translation(tile_l, tile_h, tile_data0, tile_data1);
                        for(int x = 0; x < 8; ++x) {
                            word_t shift = ((7 - x) % 4) * 2;
                            word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
                            /// @Todo: Palettes?
                            tile_map[tile_off + 16 * 8 * y + x] = color * 64;
                        }
                    }
                }
                nes_tilemap.update(reinterpret_cast<uint8_t*>(tile_map));
            }

            // Nametables
            for(int i = 0; i < 4; ++i) {
                word_t tile_l;
                word_t tile_h;
                word_t tile_data0, tile_data1;
                for(int t_y = 0; t_y < 30; ++t_y) {
                    for(int t_x = 0; t_x < 32; ++t_x) {
                        size_t tile_off = (32 * 8 * 8) * t_y + 8 * t_x;
                        word_t t = nes.ppu.get_mem(0x2000 + i * 0x400 + 32 * t_y + t_x);
                        addr_t attr_table = 0x2000 + i * 0x400 + 0x03C0 + 8 * t_y / 4 + t_x / 4;
                        word_t palette = nes.ppu.get_mem(attr_table);
                        bool   top = (t_y % 4) < 2;
                        bool   left = (t_x % 4) < 2;
                        if(top && !left)
                            palette = palette >> 2;
                        if(!top && left)
                            palette = palette >> 4;
                        if(!top && !left)
                            palette = palette >> 6;
                        palette &= 0x3;
                        for(int y = 0; y < 8; ++y) {
                            tile_l = nes.ppu.read(0x1000 * background_pattern + t * 16 + y);
                            tile_h = nes.ppu.read(0x1000 * background_pattern + t * 16 + y + 8);
                            PPU::tile_translation(tile_l, tile_h, tile_data0, tile_data1);
                            for(int x = 0; x < 8; ++x) {
                                word_t shift = ((7 - x) % 4) * 2;
                                word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
                                nametable[i][tile_off + 32 * 8 * y + x] = nes.ppu.get_color(palette, color);
                            }
                        }
                    }
                }
                /*
                for(int n = 0; n < 32 * 30; ++n)
                {
                    size_t tile_off = 8 * (n % 32) + (32 * 8 * 8) * (n / 32);
                    word_t t = nes.ppu.get_mem(0x2000 + i * 0x400 + n);
                    auto attr_col = (n % 32) / 4;
                    auto attr_row = (n / 32) / 4;
                    addr_t attr_table = 0x2000 + i * 0x400 + 0x03C0 + 8 * attr_row + attr_col;
                    for(int y = 0; y < 8; ++y)
                    {
                        tile_l = nes.ppu.read(0x1000 * background_pattern + t * 16 + y);
                        tile_h = nes.ppu.read(0x1000 * background_pattern + t * 16 + y + 8);
                        PPU::tile_translation(tile_l, tile_h, tile_data0, tile_data1);
                        for(int x = 0; x < 8; ++x)
                        {
                            word_t shift = ((7 - x) % 4) * 2;
                            word_t color = ((x > 3 ? tile_data1 : tile_data0) >> shift) & 0b11;
                            /// @Todo: Palettes?
                            nametable[i][tile_off + 32 * 8 * y + x] = color * 64;
                        }
                    }
                }
                */
                nes_nametable[i].update(reinterpret_cast<uint8_t*>(nametable[i]));
            }

            if(--speed_update == 0) {
                speed_update = 10;
                double t = timing_clock.getElapsedTime().asSeconds();
                speed = 100.0 * (double(speed_mesure_cycles) / nes.cpu.ClockRate) / (t - frame_time);
                frame_time = t;
                speed_mesure_cycles = 0;
            }
        }

        window.clear(sf::Color::Black);
        window.draw(nes_screen_sprite);

        // GUI
        ImGui::SFML::Update(window, gui_delta_clock.restart());

        if(draw_gui) {
            ImGui::Begin("Controls");
            const auto file_path = explore("./tests/");
            if(file_path != "") {
                if(!nes.load(file_path)) {
                    ImGui::Text("File not found.");
                } else {
                    path = file_path;
                    nes.reset();
                }
            }
            ImGui::End();

            ImGui::Begin("Instruction Tests");
            auto test_status = nes.cpu.read(0x6000);
            if(test_status == 0x80) {
                ImGui::Text("Test running...");
            } else {
                char        c;
                int         i = 0;
                std::string msg;
                do {
                    c = nes.cpu.read(0x6004 + i);
                    msg += c;
                    ++i;
                } while(c > 0 && i < 0x2000);
                ImGui::Text("Finished with code 0x%02x and message:", test_status);
                ImGui::Text("%s", msg.c_str());
            }
            /*
            ImGui::Text("6001 0x%02x", nes.cpu.read(0x6001));
            ImGui::Text("6002 0x%02x", nes.cpu.read(0x6002));
            ImGui::Text("6003 0x%02x", nes.cpu.read(0x6003));
            */
            ImGui::Separator();
            if(ImGui::Button("All Instructions")) {
                nes.load("tests/instr_test-v5/all_instrs.nes");
                nes.reset();
            }
            if(ImGui::Button("Official Instructions")) {
                nes.load("tests/instr_test-v5/official_only.nes");
                nes.reset();
            }
            ImGui::Text("Single ROMS");
            for(const auto& s : single_roms) {
                if(ImGui::Button(s.c_str())) {
                    nes.load("tests/instr_test-v5/rom_singles/" + s);
                    nes.reset();
                }
            }
            ImGui::End();

            ImGui::Begin("NES Status");
            ImGui::Text("Speed: %.2f\%%", speed);

            ImGui::Separator();
            ImGui::BeginTabBar("Components");

            if(ImGui::BeginTabItem("CPU")) {
                ImGui::Text("PC: 0x%04x", nes.cpu.get_pc());
                ImGui::Text("SP: 0x%04x", nes.cpu.get_sp());
                ImGui::Text("OP: 0x%02x 0x%02x 0x%02x", nes.cpu.get_next_opcode(), nes.cpu.get_next_operand0(), nes.cpu.get_next_operand1());
                ImGui::Text("A: 0x%02x", nes.cpu.get_acc());
                ImGui::Text("X: 0x%02x", nes.cpu.get_x());
                ImGui::Text("Y: 0x%02x", nes.cpu.get_y());
                ImGui::Text("PS: 0x%02x", nes.cpu.get_ps());
                ImGui::Separator();
                ImGui::Text("Flags");
                ImGui::Separator();
                ImGui::Value("Carry", nes.cpu.check(CPU::StateMask::Carry));
                ImGui::Value("Zero", nes.cpu.check(CPU::StateMask::Zero));
                ImGui::Value("Interrupt", nes.cpu.check(CPU::StateMask::Interrupt));
                ImGui::Value("Decimal", nes.cpu.check(CPU::StateMask::Decimal));
                ImGui::Value("Break", nes.cpu.check(CPU::StateMask::Break));
                ImGui::Value("Overflow", nes.cpu.check(CPU::StateMask::Overflow));
                ImGui::Value("Negative", nes.cpu.check(CPU::StateMask::Negative));
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("PPU")) {
                ImGui::Text("Scroll X: 0x%02x", nes.ppu.get_scroll_x());
                ImGui::Text("Scroll Y: 0x%02x", nes.ppu.get_scroll_y());
                ImGui::Text("TileMap");
                ImGui::Image(nes_tilemap);
                ImGui::Text("NameTables");
                for(int i = 0; i < 2; ++i) {
                    ImGui::Image(nes_nametable[2 * i], sf::Vector2f(2 * 32 * 8, 2 * 30 * 8));
                    ImGui::SameLine();
                    ImGui::Image(nes_nametable[2 * i + 1], sf::Vector2f(2 * 32 * 8, 2 * 30 * 8));
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
            ImGui::End();
        }
        ImGui::SFML::Render(window);

        window.display();
    }

    delete[] tile_map;
}
