#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "process/Instruction.h"

// Builds random programs whose expanded (loop-unrolled) length lies in [minIns, maxIns].
class InstructionGenerator {
public:
    static Program generate(const std::string& processName, uint64_t minIns, uint64_t maxIns);

private:
    static void fill(std::vector<Instruction>& out, uint64_t budget, int depth);
    static Instruction randomLeaf();
};
