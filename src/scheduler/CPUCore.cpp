#include "scheduler/CPUCore.h"

void CPUCore::assign(std::shared_ptr<Process> process) {
    process_ = std::move(process);
    quantumUsed_ = 0;
    delayCounter_ = 0;
}

std::shared_ptr<Process> CPUCore::release() {
    quantumUsed_ = 0;
    delayCounter_ = 0;
    return std::move(process_);
}

void CPUCore::tick(uint64_t cpuTick, uint64_t delaysPerExec) {
    // A process that slept or finished keeps the core until the next bookkeeping pass.
    if (process_ == nullptr || process_->state() != ProcessState::Running) return;

    ++quantumUsed_;
    if (delayCounter_ > 0) {
        --delayCounter_;  // busy-wait: holds the core without progressing
        return;
    }
    process_->step(id_, cpuTick);
    delayCounter_ = delaysPerExec;
}
