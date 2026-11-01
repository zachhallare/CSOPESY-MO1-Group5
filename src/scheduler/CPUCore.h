#pragma once

#include <cstdint>
#include <memory>

#include "process/Process.h"

// One simulated CPU core. Assignment changes only during the scheduler's bookkeeping
// phase; tick() runs on the core's own thread during the execution phase.
class CPUCore {
public:
    explicit CPUCore(int id) : id_(id) {}

    int id() const { return id_; }
    bool isIdle() const { return process_ == nullptr; }
    const std::shared_ptr<Process>& process() const { return process_; }
    uint64_t quantumUsed() const { return quantumUsed_; }

    void assign(std::shared_ptr<Process> process);
    std::shared_ptr<Process> release();

    // One CPU tick: either busy-waits out the delay or executes one instruction.
    void tick(uint64_t cpuTick, uint64_t delaysPerExec);

private:
    int id_;
    std::shared_ptr<Process> process_;
    uint64_t quantumUsed_ = 0;
    uint64_t delayCounter_ = 0;
};
