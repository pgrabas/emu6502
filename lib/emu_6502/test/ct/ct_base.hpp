#include "emu6502/assembler/compiler.hpp"
#include "emu6502/cpu/cpu.hpp"
#include <array>
#include <chrono>
#include <emu_core/base16.hpp>
#include <emu_core/clock.hpp>
#include <emu_core/memory.hpp>
#include <emu_core/program.hpp>
#include <gtest/gtest.h>

namespace emu::emu6502::test {

class ExecutionTest : public ::testing::Test {
public:
    emu::Memory memory;
    cpu::Cpu cpu{InstructionSet::NMOS6502Emu};
    emu::Clock clock;

    std::vector<uint8_t> test_data;

    ExecutionTest() {
        cpu.memory = &memory;
        cpu.clock = &clock;
        memory.clock = &clock;
    }

    void PrepareRandomTestData(size_t len = 128) {
        test_data.resize(len);
        std::generate(test_data.begin(), test_data.end(), &ExecutionTest::RandomByte);
    }

    static uint8_t RandomByte() { return rand() & 0xFF; }

    auto RunCode(const std::string &code, std::chrono::milliseconds timeout = std::chrono::seconds{1}) {
        std::cout << "-----------CODE---------------------\n" << code << "\n";
        auto program = emu::emu6502::assembler::Compiler6502::CompileString(code, InstructionSet::NMOS6502Emu);
        std::cout << "-----------PROGRAM---------------------\n" << to_string(*program) << "\n";
        memory.WriteSparse(program->sparse_binary_code.sparse_map);
        std::cout << "-----------ENTRY---------------------\n";
        auto entry_label = program->labels["TEST_ENTRY"];
        auto entry = entry_label->offset.value();
        std::cout << fmt::format("ENTRY AT {:04x}", entry) << "\n";
        cpu.reg.program_counter = entry;
        try {
            std::cout << "-----------EXECUTION---------------------\n";
            cpu.ExecuteWithTimeout(timeout);
            throw std::runtime_error("Exception was expected");
        } catch (const cpu::ExecutionHalted &e) {
            std::cout << "-----------HALTED---------------------\n";
            // std::cout << e.what() << "\n";
        }
        std::cout << "-----------RESULT---------------------\n";
        std::cout << "Cycles: " << clock.CurrentCycle() << "\n";
        return program;
    }
};

} // namespace emu::emu6502::test
