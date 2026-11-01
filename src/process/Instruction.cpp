#include "process/Instruction.h"

#include <algorithm>
#include <stdexcept>

uint64_t Instruction::expandedCount() const {
    if (op != OpType::For) return 1;
    return static_cast<uint64_t>(repeats) * ::expandedCount(body);
}

uint64_t expandedCount(const std::vector<Instruction>& code) {
    uint64_t total = 0;
    for (const auto& instruction : code) total += instruction.expandedCount();
    return total;
}

Instruction Instruction::print(uint16_t msg, VarId var) {
    Instruction i;
    i.op = OpType::Print;
    i.msg = msg;
    i.target = var;
    return i;
}

Instruction Instruction::declare(VarId var, uint16_t value) {
    Instruction i;
    i.op = OpType::Declare;
    i.target = var;
    i.lhs = Operand::literal(value);
    return i;
}

Instruction Instruction::add(VarId target, Operand lhs, Operand rhs) {
    Instruction i;
    i.op = OpType::Add;
    i.target = target;
    i.lhs = lhs;
    i.rhs = rhs;
    return i;
}

Instruction Instruction::subtract(VarId target, Operand lhs, Operand rhs) {
    Instruction i = add(target, lhs, rhs);
    i.op = OpType::Subtract;
    return i;
}

Instruction Instruction::sleep(uint8_t ticks) {
    Instruction i;
    i.op = OpType::Sleep;
    i.sleepTicks = ticks;
    return i;
}

Instruction Instruction::loop(std::vector<Instruction> body, uint16_t repeats) {
    Instruction i;
    i.op = OpType::For;
    i.body = std::move(body);
    i.repeats = repeats;
    return i;
}

VarId Program::var(const std::string& name) {
    const auto it = std::find(varNames.begin(), varNames.end(), name);
    if (it != varNames.end()) return static_cast<VarId>(it - varNames.begin());
    if (varNames.size() >= kMaxVariables) throw std::length_error("too many variables in program");
    varNames.push_back(name);
    return static_cast<VarId>(varNames.size() - 1);
}

uint16_t Program::message(const std::string& text) {
    const auto it = std::find(messages.begin(), messages.end(), text);
    if (it != messages.end()) return static_cast<uint16_t>(it - messages.begin());
    messages.push_back(text);
    return static_cast<uint16_t>(messages.size() - 1);
}
