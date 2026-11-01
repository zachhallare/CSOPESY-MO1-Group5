#include "scheduler/Scheduler.h"

#include <chrono>

#include "process/InstructionGenerator.h"

Scheduler::Scheduler(const Config& config)
    : config_(config),
      policy_(parseSchedulingPolicy(config.scheduler)),
      barrier_(static_cast<std::ptrdiff_t>(config.numCpu) + 1) {  // cores + clock
    for (uint64_t i = 0; i < config_.numCpu; ++i) cores_.emplace_back(static_cast<int>(i));
}

Scheduler::~Scheduler() { stop(); }

void Scheduler::start() {
    if (clockThread_.joinable()) return;
    for (size_t i = 0; i < cores_.size(); ++i) coreThreads_.emplace_back(&Scheduler::coreLoop, this, i);
    clockThread_ = std::thread(&Scheduler::clockLoop, this);
}

void Scheduler::stop() {
    if (!clockThread_.joinable()) return;
    stopRequested_ = true;
    clockThread_.join();
    for (auto& thread : coreThreads_) thread.join();
    coreThreads_.clear();
}

bool Scheduler::startGenerating() { return !generating_.exchange(true); }

bool Scheduler::stopGenerating() { return generating_.exchange(false); }

std::shared_ptr<Process> Scheduler::createProcess(const std::string& name) {
    {
        std::lock_guard lock(mutex_);
        if (processes_.count(name) != 0) return nullptr;
    }
    // Generate outside the lock so a long program doesn't stall the clock.
    auto program = InstructionGenerator::generate(name, config_.minIns, config_.maxIns);

    std::lock_guard lock(mutex_);
    if (processes_.count(name) != 0) return nullptr;
    return admit(name, std::move(program));
}

std::shared_ptr<Process> Scheduler::findProcess(const std::string& name) const {
    std::lock_guard lock(mutex_);
    const auto it = processes_.find(name);
    return it == processes_.end() ? nullptr : it->second;
}

Scheduler::Snapshot Scheduler::snapshot() const {
    std::lock_guard lock(mutex_);
    Snapshot snap;
    snap.totalCores = cores_.size();
    snap.finished = finished_;
    for (const auto& core : cores_) {
        if (core.isIdle()) continue;
        // A process that finished this tick still holds its core until bookkeeping.
        if (core.process()->isFinished()) {
            snap.finished.push_back(core.process());
        } else {
            snap.running.emplace_back(core.id(), core.process());
        }
    }
    return snap;
}

void Scheduler::clockLoop() {
    using Clock = std::chrono::steady_clock;
    const auto tickPeriod = std::chrono::milliseconds(config_.tickDelayMs);
    // Pacing against a deadline (not a fixed sleep) keeps the average rate correct even
    // though the OS sleep granularity can be coarser than one tick.
    auto deadline = Clock::now();

    while (true) {
        if (stopRequested_) {
            stopping_ = true;
            barrier_.arrive_and_wait();  // releases the cores so they can see stopping_
            return;
        }
        bookkeeping();
        barrier_.arrive_and_wait();  // start of execution phase
        barrier_.arrive_and_wait();  // end of execution phase

        if (tickPeriod.count() > 0) {
            deadline += tickPeriod;
            const auto now = Clock::now();
            if (now - deadline > std::chrono::milliseconds(100)) deadline = now;  // don't burst after a stall
            std::this_thread::sleep_until(deadline);
        }
    }
}

void Scheduler::coreLoop(size_t coreIndex) {
    while (true) {
        barrier_.arrive_and_wait();
        if (stopping_) return;
        cores_[coreIndex].tick(cpuCycles_, config_.delaysPerExec);
        barrier_.arrive_and_wait();
    }
}

void Scheduler::bookkeeping() {
    std::lock_guard lock(mutex_);
    const uint64_t tick = ++cpuCycles_;

    if (generating_ && tick % config_.batchProcessFreq == 0) {
        const std::string name = nextBatchName();
        admit(name, InstructionGenerator::generate(name, config_.minIns, config_.maxIns));
    }

    // Free cores whose process finished, went to sleep, or used up its quantum.
    for (auto& core : cores_) {
        if (core.isIdle()) continue;
        switch (core.process()->state()) {
            case ProcessState::Finished:
                finished_.push_back(core.release());
                break;
            case ProcessState::Sleeping:
                sleeping_.push_back(core.release());
                break;
            default:
                if (policy_ == SchedulingPolicy::RoundRobin &&
                    core.quantumUsed() >= config_.quantumCycles) {
                    auto process = core.release();
                    process->setState(ProcessState::Ready);
                    ready_.push_back(std::move(process));
                }
                break;
        }
    }

    for (auto it = sleeping_.begin(); it != sleeping_.end();) {
        if ((*it)->wakeTick() <= tick) {
            (*it)->setState(ProcessState::Ready);
            ready_.push_back(*it);
            it = sleeping_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& core : cores_) {
        if (ready_.empty()) break;
        if (!core.isIdle()) continue;
        auto process = ready_.front();
        ready_.pop_front();
        process->setState(ProcessState::Running);
        core.assign(std::move(process));
    }
}

std::shared_ptr<Process> Scheduler::admit(const std::string& name, Program program) {
    auto process = std::make_shared<Process>(nextId_++, name, std::move(program));
    processes_.emplace(name, process);
    ready_.push_back(process);
    return process;
}

std::string Scheduler::nextBatchName() {
    while (true) {
        const uint64_t index = nextBatchIndex_++;
        std::string name = (index < 10 ? "p0" : "p") + std::to_string(index);
        if (processes_.count(name) == 0) return name;  // skip names taken via screen -s
    }
}
