#include "process/Process.h"

#include <algorithm>
#include <chrono>

#include "util/TimeUtil.h"

namespace {

constexpr int64_t kUint16Max = 65535;

uint16_t clampUint16(int64_t value) {
    return static_cast<uint16_t>(std::clamp<int64_t>(value, 0, kUint16Max));
}

}  // namespace

Process::Process(uint64_t id, std::string name, Program program)
    : id_(id),
      name_(std::move(name)),
      createdAt_(util::currentTimestamp()),
      program_(std::move(program)),
      total_(program_.expandedCount()),
      vars_(program_.varNames.size(), 0) {
    frames_.push_back({&program_.code, 0, 1});
    if (currentLeaf() == nullptr) state_ = ProcessState::Finished;
}

void Process::step(int coreId, uint64_t tick) {
    std::lock_guard lock(mutex_);
    const Instruction* leaf = currentLeaf();
    if (leaf == nullptr) {
        state_ = ProcessState::Finished;
        return;
    }

    execute(*leaf, coreId, tick);
    ++frames_.back().pc;
    ++executed_;

    if (currentLeaf() == nullptr) {
        // Variables live until the process finishes.
        vars_.clear();
        vars_.shrink_to_fit();
        state_ = ProcessState::Finished;
    }
}

const Instruction* Process::currentLeaf() {
    while (!frames_.empty()) {
        Frame& top = frames_.back();
        if (top.pc >= top.list->size()) {
            if (top.remaining > 1) {
                --top.remaining;
                top.pc = 0;
                continue;
            }
            frames_.pop_back();
            if (!frames_.empty()) ++frames_.back().pc;  // step past the finished FOR
            continue;
        }

        const Instruction& instruction = (*top.list)[top.pc];
        if (instruction.op != OpType::For) return &instruction;

        if (instruction.repeats == 0 || instruction.body.empty()) {
            ++top.pc;
        } else {
            frames_.push_back({&instruction.body, 0, instruction.repeats});
        }
    }
    return nullptr;
}

void Process::execute(const Instruction& instruction, int coreId, uint64_t tick) {
    switch (instruction.op) {
        case OpType::Print: {
            const int32_t value =
                instruction.target == kNoVar ? -1 : vars_[instruction.target];
            const std::time_t now =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            logs_.push_back({now, coreId, instruction.msg, value});
            break;
        }
        case OpType::Declare:
            vars_[instruction.target] = instruction.lhs.value;
            break;
        case OpType::Add:
            vars_[instruction.target] = clampUint16(static_cast<int64_t>(resolve(instruction.lhs)) +
                                                    resolve(instruction.rhs));
            break;
        case OpType::Subtract:
            vars_[instruction.target] = clampUint16(static_cast<int64_t>(resolve(instruction.lhs)) -
                                                    resolve(instruction.rhs));
            break;
        case OpType::Sleep:
            if (instruction.sleepTicks > 0) {
                // Skip exactly sleepTicks ticks, then run again on the next one.
                wakeTick_ = tick + instruction.sleepTicks + 1;
                state_ = ProcessState::Sleeping;
            }
            break;
        case OpType::For:
            break;  // never a leaf; currentLeaf() enters loop bodies
    }
}

uint16_t Process::resolve(const Operand& operand) const {
    return operand.isVar() ? vars_[operand.var] : operand.value;
}

std::vector<std::string> Process::formattedLogs() const {
    std::lock_guard lock(mutex_);
    std::vector<std::string> lines;
    lines.reserve(logs_.size());
    for (const auto& log : logs_) {
        std::string message = program_.messages[log.msg];
        if (log.varValue >= 0) message += std::to_string(log.varValue);
        lines.push_back("(" + util::formatTimestamp(log.time) + ") Core:" +
                        std::to_string(log.coreId) + " \"" + message + "\"");
    }
    return lines;
}

uint16_t Process::variable(const std::string& name) const {
    std::lock_guard lock(mutex_);
    const auto& names = program_.varNames;
    const auto it = std::find(names.begin(), names.end(), name);
    const size_t index = static_cast<size_t>(it - names.begin());
    return index < vars_.size() ? vars_[index] : 0;
}
