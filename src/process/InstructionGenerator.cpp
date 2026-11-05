#include "process/InstructionGenerator.h"

#include <algorithm>
#include <array>

#include "util/Random.h"

namespace {

constexpr int kMaxForDepth = 3;
constexpr uint64_t kMaxBodySize = 20;
constexpr uint64_t kSmallRepeats = 5;
constexpr uint64_t kLargeRepeats = 65535;
// Above this budget a FOR is forced, so huge max-ins values stay a small tree.
constexpr uint64_t kLargeBudget = 10000;
constexpr uint64_t kLoopChancePercent = 10;
constexpr uint64_t kMaxLiteral = 500;
constexpr uint64_t kMaxSleepTicks = 5;

// Interned first by generate(), so their ids are 0..size-1.
const std::array<const char*, 6> kVariables = {"x", "y", "z", "a", "b", "c"};
// Message id 0 is always "Hello world from <name>!".
constexpr uint16_t kHelloMessage = 0;

VarId randomVariable() { return static_cast<VarId>(util::randRange(0, kVariables.size() - 1)); }

Operand randomOperand() {
    if (util::randRange(0, 1) == 0) return Operand::variable(randomVariable());
    return Operand::literal(static_cast<uint16_t>(util::randRange(0, kMaxLiteral)));
}

}  // namespace

Program InstructionGenerator::generate(const std::string& processName, uint64_t minIns,
                                       uint64_t maxIns, ProgramPattern pattern) {
    const uint64_t length = util::randRange(minIns, maxIns);
    if (pattern == ProgramPattern::PrintAdd) return generatePrintAdd(length);

    Program program;
    program.message("Hello world from " + processName + "!");
    for (const char* name : kVariables) program.var(name);
    fill(program.code, length, 0);
    return program;
}

Program InstructionGenerator::generatePrintAdd(uint64_t length) {
    Program program;
    const uint16_t valueFrom = program.message("Value from: ");
    const VarId x = program.var("x");  // every process starts with x = 0
    program.code.reserve(length);
    for (uint64_t i = 0; i < length; ++i) {
        if (i % 2 == 0) {
            program.code.push_back(Instruction::print(valueFrom, x));
        } else {
            const auto amount = static_cast<uint16_t>(util::randRange(1, 10));
            program.code.push_back(Instruction::add(x, Operand::variable(x), Operand::literal(amount)));
        }
    }
    return program;
}

void InstructionGenerator::fill(std::vector<Instruction>& out, uint64_t budget, int depth) {
    while (budget > 0) {
        const bool canLoop = depth < kMaxForDepth && budget >= 2;
        const bool wantLoop =
            canLoop && (budget > kLargeBudget || util::randRange(1, 100) <= kLoopChancePercent);

        if (!wantLoop) {
            out.push_back(randomLeaf());
            --budget;
            continue;
        }

        // bodySize <= budget / 2 guarantees room for at least 2 repeats.
        const uint64_t bodySize = util::randRange(1, std::min(budget / 2, kMaxBodySize));
        const uint64_t repeatCap = budget > kLargeBudget ? kLargeRepeats : kSmallRepeats;
        const uint64_t repeats = util::randRange(2, std::min(budget / bodySize, repeatCap));

        std::vector<Instruction> body;
        fill(body, bodySize, depth + 1);
        out.push_back(Instruction::loop(std::move(body), static_cast<uint16_t>(repeats)));
        budget -= bodySize * repeats;
    }
}

Instruction InstructionGenerator::randomLeaf() {
    // Weights out of 10: PRINT 3, DECLARE 2, ADD 2, SUBTRACT 2, SLEEP 1.
    const uint64_t roll = util::randRange(0, 9);
    if (roll < 3) return Instruction::print(kHelloMessage);
    if (roll < 5) {
        return Instruction::declare(randomVariable(),
                                    static_cast<uint16_t>(util::randRange(0, kMaxLiteral)));
    }
    if (roll < 7) return Instruction::add(randomVariable(), randomOperand(), randomOperand());
    if (roll < 9) return Instruction::subtract(randomVariable(), randomOperand(), randomOperand());
    return Instruction::sleep(static_cast<uint8_t>(util::randRange(1, kMaxSleepTicks)));
}
