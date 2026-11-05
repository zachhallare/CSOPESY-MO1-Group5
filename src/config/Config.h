#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Scheduler and process parameters read from config.txt by "initialize".
struct Config {
    uint64_t numCpu = 4;
    std::string scheduler = "rr";
    uint64_t quantumCycles = 5;
    uint64_t batchProcessFreq = 1;
    uint64_t minIns = 1000;
    uint64_t maxIns = 2000;
    uint64_t delaysPerExec = 0;
    // Not part of the spec: real-time pause between CPU ticks so progress is watchable.
    uint64_t tickDelayMs = 10;
    // Not part of the spec: real-time length of one batch-generator tick. batch-process-freq
    // counts these, so generation is paced independently of how fast the cores run.
    uint64_t batchTickMs = 250;
    // Not part of the spec: "random", or "print-add" for alternating
    // PRINT("Value from: " + x) / ADD(x, x, 1-10) programs.
    std::string instructionPattern = "random";

    // Parses a whitespace-separated key/value file. On failure, returns false and fills
    // `error`; recoverable oddities such as unknown keys go to `warnings`.
    static bool load(const std::string& path, Config& out, std::string& error,
                     std::vector<std::string>& warnings);
};
