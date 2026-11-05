#include "config/Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include "util/StringUtil.h"

namespace {

constexpr uint64_t kMaxValue = 1ULL << 32;

bool parseNumber(const std::string& text, uint64_t& out) {
    if (text.empty() || text.size() > 19) return false;
    if (!std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return false;
    }
    out = std::stoull(text);
    return true;
}

bool checkRange(const std::string& key, uint64_t value, uint64_t lo, uint64_t hi, std::string& error) {
    if (value >= lo && value <= hi) return true;
    error = "'" + key + "' must be in [" + std::to_string(lo) + ", " + std::to_string(hi) +
            "], got " + std::to_string(value) + ".";
    return false;
}

}  // namespace

bool Config::load(const std::string& path, Config& out, std::string& error,
                  std::vector<std::string>& warnings) {
    std::ifstream file(path);
    if (!file) {
        error = "Could not open '" + path + "'.";
        return false;
    }

    Config cfg;
    std::string key;
    std::string value;
    while (file >> key) {
        if (!(file >> value)) {
            error = "Missing value for '" + key + "'.";
            return false;
        }
        key = util::toLower(key);
        value = util::stripQuotes(value);

        if (key == "scheduler") {
            cfg.scheduler = util::toLower(value);
            continue;
        }
        if (key == "instruction-pattern") {
            cfg.instructionPattern = util::toLower(value);
            continue;
        }

        uint64_t* field = nullptr;
        if (key == "num-cpu") field = &cfg.numCpu;
        else if (key == "quantum-cycles") field = &cfg.quantumCycles;
        else if (key == "batch-process-freq") field = &cfg.batchProcessFreq;
        else if (key == "min-ins") field = &cfg.minIns;
        else if (key == "max-ins") field = &cfg.maxIns;
        else if (key == "delays-per-exec" || key == "delay-per-exec") field = &cfg.delaysPerExec;
        else if (key == "tick-delay-ms") field = &cfg.tickDelayMs;
        else if (key == "batch-tick-ms") field = &cfg.batchTickMs;

        if (field == nullptr) {
            warnings.push_back("Ignoring unknown parameter '" + key + "'.");
            continue;
        }
        if (!parseNumber(value, *field)) {
            error = "Invalid value '" + value + "' for '" + key + "'.";
            return false;
        }
    }

    if (cfg.scheduler != "fcfs" && cfg.scheduler != "rr") {
        error = "'scheduler' must be \"fcfs\" or \"rr\", got \"" + cfg.scheduler + "\".";
        return false;
    }
    if (cfg.instructionPattern != "random" && cfg.instructionPattern != "print-add") {
        error = "'instruction-pattern' must be \"random\" or \"print-add\", got \"" +
                cfg.instructionPattern + "\".";
        return false;
    }
    if (!checkRange("num-cpu", cfg.numCpu, 1, 128, error) ||
        !checkRange("quantum-cycles", cfg.quantumCycles, 1, kMaxValue, error) ||
        !checkRange("batch-process-freq", cfg.batchProcessFreq, 1, kMaxValue, error) ||
        !checkRange("min-ins", cfg.minIns, 1, kMaxValue, error) ||
        !checkRange("max-ins", cfg.maxIns, 1, kMaxValue, error) ||
        !checkRange("delay-per-exec", cfg.delaysPerExec, 0, kMaxValue, error) ||
        !checkRange("tick-delay-ms", cfg.tickDelayMs, 0, 10000, error) ||
        !checkRange("batch-tick-ms", cfg.batchTickMs, 1, 60000, error)) {
        return false;
    }
    if (cfg.minIns > cfg.maxIns) {
        error = "'min-ins' (" + std::to_string(cfg.minIns) + ") cannot exceed 'max-ins' (" +
                std::to_string(cfg.maxIns) + ").";
        return false;
    }

    out = cfg;
    return true;
}
