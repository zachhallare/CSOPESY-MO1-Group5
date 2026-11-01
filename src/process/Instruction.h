#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class OpType : uint8_t { Print, Declare, Add, Subtract, Sleep, For };

// Variables are referred to by an index into Program::varNames. This keeps
// instructions small, since thousands of processes may be alive at once.
using VarId = uint8_t;
constexpr VarId kNoVar = 0xFF;
constexpr size_t kMaxVariables = kNoVar;  // ids 0..254

// A variable reference or a uint16 literal.
struct Operand {
    VarId var = kNoVar;
    uint16_t value = 0;

    bool isVar() const { return var != kNoVar; }

    static Operand variable(VarId id) { return {id, 0}; }
    static Operand literal(uint16_t v) { return {kNoVar, v}; }
};

struct Instruction {
    OpType op = OpType::Print;
    VarId target = kNoVar;   // DECLARE / ADD / SUBTRACT destination; PRINT's optional variable
    uint8_t sleepTicks = 0;
    uint16_t repeats = 0;    // FOR
    uint16_t msg = 0;        // PRINT: index into Program::messages
    Operand lhs;             // DECLARE value, or first ADD / SUBTRACT operand
    Operand rhs;             // second ADD / SUBTRACT operand
    std::vector<Instruction> body;  // FOR body

    // Number of leaf instructions executed when this runs, with FOR loops unrolled.
    uint64_t expandedCount() const;

    static Instruction print(uint16_t msg, VarId var = kNoVar);
    static Instruction declare(VarId var, uint16_t value);
    static Instruction add(VarId target, Operand lhs, Operand rhs);
    static Instruction subtract(VarId target, Operand lhs, Operand rhs);
    static Instruction sleep(uint8_t ticks);
    static Instruction loop(std::vector<Instruction> body, uint16_t repeats);
};

uint64_t expandedCount(const std::vector<Instruction>& code);

// A process's instructions plus the name tables they index into.
struct Program {
    std::vector<Instruction> code;
    std::vector<std::string> varNames;
    std::vector<std::string> messages;

    // Returns the id for `name`, adding it if new. At most kMaxVariables names.
    VarId var(const std::string& name);
    // Returns the index for `text`, adding it if new.
    uint16_t message(const std::string& text);

    uint64_t expandedCount() const { return ::expandedCount(code); }
};
