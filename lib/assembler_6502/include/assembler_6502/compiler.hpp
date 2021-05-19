#pragma once

#include "cpu_6502/instruction_set.hpp"
#include "emu_core/program.hpp"
#include "tokenizer.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace emu::assembler6502 {

struct CompilationContext;

struct InstructionParsingInfo {
    std::unordered_map<cpu6502::AddressMode, cpu6502::OpcodeInfo> variants;
};

class Compiler6502 {
public:
    using InstructionSet = cpu6502::InstructionSet;

    Compiler6502(InstructionSet cpu_instruction_set = InstructionSet::Default);

    std::unique_ptr<Program> Compile(Tokenizer &tokenizer);

    static std::unique_ptr<Program> CompileString(std::string text,
                                                  InstructionSet cpu_instruction_set = InstructionSet::Default);
    static std::unique_ptr<Program> CompileFile(const std::string &file,
                                                InstructionSet cpu_instruction_set = InstructionSet::Default);

private:
    std::unordered_map<std::string_view, InstructionParsingInfo> instruction_set;

    void ProcessLine(CompilationContext &context, LineTokenizer &line);
};

} // namespace emu::assembler6502