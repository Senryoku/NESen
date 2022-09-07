#include <iomanip>
#include <queue>
#include <sstream>

#include <core/NES.hpp>
#include <tools/CommandLine.hpp>

bool debug = false;
bool step = true;
bool real_speed = true;

struct test_log {
    std::string executed;
    std::string expected;
};

template<typename T>
struct HGenUpper {
    T v;
    HGenUpper(T _t) : v(_t) {}
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const HGenUpper<T>& t) {
    return os << std::uppercase << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << (int)t.v;
}

using H = HGenUpper<addr_t>;
using H8 = HGenUpper<word_t>;

int main(int argc, char* argv[]) {
    config::set_folder(argv[0]);

    NES         nes;
    std::string path = "tests/nestest.nes";
    std::string test_full_log_path = "tests/nestest.log";

    std::queue<test_log> log_queue;

    if(!nes.load(path)) {
        Log::error("Error loading '", path, "'. Exiting...");
        return 1;
    }

    nes.reset();
    nes.cpu.set_pc(0xC000);

    std::ifstream full_log_file;
    full_log_file.open(test_full_log_path, std::ios::binary);
    if(!full_log_file.is_open()) {
        Log::error("Error loading '", test_full_log_path, "'. Exiting...");
        return 1;
    }

    int  instr_count = 0;
    int  error_count = 0;
    int  expected_addr = 0;
    int  expected_opcode;
    int  expected_operande0;
    int  expected_operande1;
    char disassembly[49 - 15 + 1];
    int  expected_a;
    int  expected_x;
    int  expected_y;
    int  expected_p;
    int  expected_sp;
    int  expected_cyc;
    do {
        instr_count++;

        std::stringstream exec_ss;
        exec_ss << H(nes.cpu.get_pc()) << "  " << H8(nes.cpu.get_next_opcode()) << " " << H8(nes.cpu.get_next_operand0()) << " " << H8(nes.cpu.get_next_operand1()) << "  \t\t\t\t"
                << "   A:" << H8(nes.cpu.get_acc()) << " X:" << H8(nes.cpu.get_x()) << " Y:" << H8(nes.cpu.get_y()) << " P:" << H8(nes.cpu.get_ps())
                << " SP:" << H8(nes.cpu.get_sp());

        char full_log[255];
        full_log_file.getline(full_log, 255);
        std::stringstream ss(full_log);
        ss >> std::hex >> expected_addr;
        ss >> std::hex >> expected_opcode;
        if(CPU::instr_length[expected_opcode] >= 1)
            ss >> std::hex >> expected_operande0;
        if(CPU::instr_length[expected_opcode] >= 2)
            ss >> std::hex >> expected_operande1;
        ss.get(disassembly, 49 - 15);
        ss.ignore(2);
        ss >> expected_a;
        ss.ignore(3);
        ss >> expected_x;
        ss.ignore(3);
        ss >> expected_y;
        ss.ignore(3);
        ss >> expected_p;
        ss.ignore(4);
        ss >> expected_sp;
        ss.ignore(5);
        ss >> expected_cyc;

        log_queue.push(test_log{exec_ss.str(), full_log});
        if(log_queue.size() > 20)
            log_queue.pop();

        if(nes.cpu.get_pc() != expected_addr) {
            Log::error("[", std::dec, instr_count, "]\tERROR! Got ", H(nes.cpu.get_pc()), " expected ", H(expected_addr));
            Log::error(" >  A:", H8(nes.cpu.get_acc()), " X:", H8(nes.cpu.get_x()), " Y:", H8(nes.cpu.get_y()), " PS:", H8(nes.cpu.get_ps()), " SP:", H(nes.cpu.get_sp()));

            while(!log_queue.empty()) {
                auto l = log_queue.front();
                log_queue.pop();
                Log::error(l.executed);
                Log::info(l.expected);
            }

            nes.cpu.set_pc(expected_addr);
            error_count++;
        } else {
            /*
            Log::info("[", std::dec, instr_count, "]\t", H(nes.cpu.get_pc()), " "
                                            , H8(nes.cpu.read(nes.cpu.get_pc())), " "
                                            , H8(nes.cpu.read(nes.cpu.get_pc() + 1)), " "
                                            , H8(nes.cpu.read(nes.cpu.get_pc() + 2))
                                            , "\tA:", H8(nes.cpu.get_acc())
                                            , " X:", H8(nes.cpu.get_x())
                                            , " Y:", H8(nes.cpu.get_y())
                                            , " PS:", H8(nes.cpu.get_ps())
                                            , " SP:", H(nes.cpu.get_sp()));
            */
        }

        nes.step();
    } while(error_count == 0);

    return error_count;
}
