#include <iomanip>
#include <sstream>

#include <core/NES.hpp>
#include <tools/CommandLine.hpp>

// Timing
uint64_t elapsed_cycles = 0;
uint64_t speed_mesure_cycles = 0;
size_t   frame_count = 0;

const std::vector<std::string> single_roms{"01-basics.nes", "02-implied.nes", "03-immediate.nes", "04-zero_page.nes", "05-zp_xy.nes", "06-absolute.nes",
                                           "07-abs_xy.nes", "08-ind_x.nes",   "09-ind_y.nes",     "10-branches.nes",  "11-stack.nes", "12-jmp_jsr.nes",
                                           "13-rts.nes",    "14-rti.nes",     "15-brk.nes",       "16-special.nes"};

void run(const std::string& path) {
    NES nes;
    nes.cpu.controller_callbacks[0] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[1] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[2] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[3] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[4] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[5] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[6] = [&]() -> bool { return false; };
    nes.cpu.controller_callbacks[7] = [&]() -> bool { return false; };
    if(!nes.load(path)) {
        Log::error("Error loading '", path, "'.");
        return;
    }
    nes.reset();
    Log::print("Running ", path, "...");
    word_t       test_status = 0x80;
    const word_t magic[3] = {0xDE, 0xB0, 0x61};
    bool         valid_status = false;
    do {
        nes.step();

        test_status = nes.cpu.read(0x6000);
        valid_status = nes.cpu.read(0x6001) == magic[0] && nes.cpu.read(0x6002) == magic[1] && nes.cpu.read(0x6003) == magic[2];
    } while(test_status == 0x80 || !valid_status);

    char        c;
    int         i = 0;
    std::string msg;
    do {
        c = nes.cpu.read(0x6004 + i);
        msg += c;
        ++i;
    } while(c > 0 && i < 0x2000);

    if(test_status == 0x00) {
        Log::success("Succeeded with code ", Hexa8(test_status), " and message: ", msg);
    } else {
        Log::error("Error with code ", Hexa8(test_status), " and message: ", msg);
    }
}

int main(int argc, char* argv[]) {
    config::set_folder(argv[0]);

    for(const auto& p : single_roms) {
        run("tests/instr_test-v5/rom_singles/" + p);
    }
    run("tests/instr_test-v5/official_only.nes");
    run("tests/instr_test-v5/all_instrs.nes");
}
