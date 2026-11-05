#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "process/Instruction.h"

enum class ProgramPattern {
    Random,    // mixed instructions with nested FOR loops
    PrintAdd,  // PRINT("Value from: " + x), ADD(x, x, 1-10), PRINT, ADD, ...
};

// Expects a lower-case name already validated by Config ("random" or "print-add").
inline ProgramPattern parseProgramPattern(const std::string& name) {
    return name == "print-add" ? ProgramPattern::PrintAdd : ProgramPattern::Random;
}

// Builds programs whose expanded (loop-unrolled) length lies in [minIns, maxIns].
class InstructionGenerator {
public:
    static Program generate(const std::string& processName, uint64_t minIns, uint64_t maxIns,
                            ProgramPattern pattern = ProgramPattern::Random);

private:
    static Program generatePrintAdd(uint64_t length);
    static void fill(std::vector<Instruction>& out, uint64_t budget, int depth);
    static Instruction randomLeaf();
};
