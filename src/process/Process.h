#pragma once

#include <atomic>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

#include "process/Instruction.h"

enum class ProcessState { Ready, Running, Sleeping, Finished };

// Stored compactly; formatted into text only when someone views the logs.
struct LogEntry {
    std::time_t time;
    int coreId;
    uint16_t msg;       // index into Program::messages
    int32_t varValue;   // value appended to the message, or -1 for none
};

// A process owns its program and execution state. Only one core runs it at a time;
// the mutex protects logs and variables from concurrent reads by the CLI.
class Process {
public:
    Process(uint64_t id, std::string name, Program program);
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Executes the next leaf instruction on `coreId` during CPU tick `tick`.
    void step(int coreId, uint64_t tick);

    uint64_t id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& createdAt() const { return createdAt_; }
    uint64_t totalInstructions() const { return total_; }
    uint64_t executedInstructions() const { return executed_; }

    ProcessState state() const { return state_; }
    void setState(ProcessState state) { state_ = state; }
    bool isFinished() const { return state_ == ProcessState::Finished; }
    // First CPU tick at which a sleeping process may run again.
    uint64_t wakeTick() const { return wakeTick_; }

    // Log lines formatted as: (MM/DD/YYYY HH:MM:SSAM) Core:N "message"
    std::vector<std::string> formattedLogs() const;
    // Current value of a variable (0 if undeclared); used by tests.
    uint16_t variable(const std::string& name) const;

private:
    struct Frame {
        const std::vector<Instruction>* list;
        size_t pc;
        uint64_t remaining;  // passes left over `list`, including the current one
    };

    // Unwinds finished frames and enters FOR bodies until the top frame points at a
    // leaf. Returns nullptr when the program is done. Caller holds mutex_.
    const Instruction* currentLeaf();
    void execute(const Instruction& instruction, int coreId, uint64_t tick);
    uint16_t resolve(const Operand& operand) const;

    const uint64_t id_;
    const std::string name_;
    const std::string createdAt_;
    const Program program_;
    const uint64_t total_;

    mutable std::mutex mutex_;
    std::vector<Frame> frames_;
    // Indexed by VarId. Every variable starts at 0, which matches auto-declaration.
    std::vector<uint16_t> vars_;
    std::vector<LogEntry> logs_;

    std::atomic<uint64_t> executed_{0};
    std::atomic<ProcessState> state_{ProcessState::Ready};
    std::atomic<uint64_t> wakeTick_{0};
};
