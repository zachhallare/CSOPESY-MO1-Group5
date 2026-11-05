#pragma once

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config/Config.h"
#include "process/InstructionGenerator.h"
#include "process/Process.h"
#include "scheduler/CPUCore.h"
#include "scheduler/SchedulingPolicy.h"

// Real-time CPU scheduler. A clock thread advances the CPU tick and does all queue
// bookkeeping; one thread per core executes instructions. A barrier keeps each tick in
// two phases (bookkeeping, then execution) so the two never overlap.
class Scheduler {
public:
    struct Snapshot {
        size_t totalCores = 0;
        std::vector<std::pair<int, std::shared_ptr<Process>>> running;  // (core id, process)
        std::vector<std::shared_ptr<Process>> finished;                 // completion order
    };

    explicit Scheduler(const Config& config);
    ~Scheduler();
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start();
    void stop();

    // Both return false if generation was already in the requested state.
    bool startGenerating();
    bool stopGenerating();

    // Creates and enqueues a process. Returns nullptr if the name is taken.
    std::shared_ptr<Process> createProcess(const std::string& name);
    std::shared_ptr<Process> findProcess(const std::string& name) const;
    Snapshot snapshot() const;

private:
    void clockLoop();
    void coreLoop(size_t coreIndex);
    void bookkeeping();
    // Advances the real-time batch clock and admits a process every batch-process-freq
    // batch ticks. Caller holds mutex_.
    void generateBatch();

    // Caller holds mutex_.
    std::shared_ptr<Process> admit(const std::string& name, Program program);
    std::string nextBatchName();

    const Config config_;
    const SchedulingPolicy policy_;
    const ProgramPattern pattern_;
    std::vector<CPUCore> cores_;

    mutable std::mutex mutex_;
    std::deque<std::shared_ptr<Process>> ready_;
    std::vector<std::shared_ptr<Process>> sleeping_;
    std::vector<std::shared_ptr<Process>> finished_;
    std::unordered_map<std::string, std::shared_ptr<Process>> processes_;
    uint64_t nextId_ = 1;
    uint64_t nextBatchIndex_ = 1;
    // Batch clock, touched only by the clock thread.
    std::chrono::steady_clock::time_point nextBatchTick_;
    uint64_t batchTicks_ = 0;
    std::atomic<bool> restartBatchClock_{false};

    std::atomic<uint64_t> cpuCycles_{0};
    std::atomic<bool> generating_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> stopping_{false};

    std::barrier<> barrier_;
    std::thread clockThread_;
    std::vector<std::thread> coreThreads_;
};
