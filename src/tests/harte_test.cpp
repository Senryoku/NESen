/* Runs single instruction tests from https://github.com/TomHarte/ProcessorTests/tree/main/nes6502 */

#include <bitset>
#include <filesystem>
#include <format>

#include <core/NES.hpp>
#include <tools/CommandLine.hpp>

#include <json.hpp>

using namespace nlohmann;

int main(int argc, char* argv[]) {
    config::set_folder(argv[0]);

    NES nes(0x10000);
    nes.cartridge.load_test();
    nes.reset();

    std::vector<std::string> failing_tests;

    const auto run_file = [&](auto test_path) {
        std::cout << "Running file " << test_path << "... ";
        std::ifstream test_file(test_path);
        json          test_data = json::parse(test_file);
        for(auto i = 0; i < test_data.size(); ++i) {
            std::cout << "\rRunning file " << test_path << "... " << i + 1 << "/" << test_data.size();
            auto it = test_data[i];

            // Load initial state
            const auto& initial_state = it["initial"];
            nes.cpu.set_state(initial_state["pc"], initial_state["a"], initial_state["x"], initial_state["y"], initial_state["s"], initial_state["p"]);
            for(auto& it_ram : initial_state["ram"])
                nes.cpu.write(it_ram[0], it_ram[1]);

            // Run instruction
            nes.cpu.step();
            // Check final state
            const auto& final_state = it["final"];
            bool        ram_error = false;
            for(auto& it_ram : final_state["ram"]) {
                if(nes.cpu.read(it_ram[0]) != it_ram[1]) {
                    ram_error = true;
                    break;
                }
            }
            if(ram_error || final_state["pc"] != nes.cpu.get_pc() || final_state["a"] != nes.cpu.get_acc() || final_state["x"] != nes.cpu.get_x() ||
               final_state["y"] != nes.cpu.get_y() || final_state["s"] != nes.cpu.get_sp() || final_state["p"] != nes.cpu.get_ps()) {
                std::cout << "\n  Error running test " << it["name"] << "\n";
                std::cout << "                 \tExpect.\tActual\n";

                const auto print_status = [&](auto& name, auto expected, auto value) {
                    if(value != static_cast<int>(expected))
                        std::cout << "\033[31;1m";
                    std::cout << "           " << name << "    \t" << expected << "\t" << static_cast<int>(value) << "\033[0m\n";
                };

                print_status("PC", final_state["pc"], nes.cpu.get_pc());
                print_status("A ", final_state["a"], nes.cpu.get_acc());
                print_status("X ", final_state["x"], nes.cpu.get_x());
                print_status("Y ", final_state["y"], nes.cpu.get_y());
                print_status("SP", final_state["s"], nes.cpu.get_sp());
                print_status("PS", final_state["p"], nes.cpu.get_ps());
                std::cout << "               " << std::bitset<8>(final_state["p"]) << "\n";
                std::cout << "               " << std::bitset<8>(nes.cpu.get_ps()) << "\n";

                std::cout << std::endl;

                for(auto& it_ram : final_state["ram"]) {
                    if(it_ram[1] != static_cast<int>(nes.cpu.read(it_ram[0])))
                        std::cout << "\033[31;1m";
                    std::cout << "           " << it_ram[0] << "  \t" << static_cast<int>(nes.cpu.read(it_ram[0])) << "\t" << it_ram[1] << "\033[0m\n";
                }

                failing_tests.push_back(it["name"]);

                return -1;
            }
        }
        std::cout << " \033[32;1mOk\033[0m\n";
        return 0;
    };

    if(get_file(argc, argv)) {
        run_file(std::format("tests/ProcessorTests/nes6502/v1/{}.json", get_file(argc, argv)));
    } else {
        const std::filesystem::path tests_path{"tests/ProcessorTests/nes6502/v1/"};
        for(auto const& test_path : std::filesystem::directory_iterator{tests_path})
            run_file(test_path.path());
    }

    if(!failing_tests.empty()) {
        std::cout << "Failed tests (Max one per opcode):" << std::endl;
        for(const auto& s : failing_tests)
            std::cout << s << std::endl;
    }
}