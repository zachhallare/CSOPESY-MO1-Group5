// Self-contained checks for instruction semantics, the generator, and config parsing.
// Run: build/csopesy_tests  (exit code 0 = all passed)

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "config/Config.h"
#include "process/InstructionGenerator.h"
#include "process/Process.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (!condition) {
        ++failures;
        std::cout << "FAIL: " << what << "\n";
    }
}

// Steps a process to completion (ignoring sleep state); returns ticks used.
uint64_t runToEnd(Process& process) {
    uint64_t tick = 0;
    while (!process.isFinished() && tick < 10'000'000) {
        process.setState(ProcessState::Running);
        process.step(0, ++tick);
    }
    return tick;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void testClamping() {
    Program upProgram;
    const VarId x = upProgram.var("x");
    upProgram.code = {Instruction::declare(x, 65530),
                      Instruction::add(x, Operand::variable(x), Operand::literal(100)),
                      Instruction::print(upProgram.message("x is "), x)};
    Process up(1, "up", std::move(upProgram));
    up.setState(ProcessState::Running);
    up.step(0, 1);
    up.step(0, 2);
    check(up.variable("x") == 65535, "ADD clamps at 65535");

    Program downProgram;
    const VarId y = downProgram.var("y");
    downProgram.code = {Instruction::subtract(y, Operand::literal(0), Operand::literal(5)),
                        Instruction::print(downProgram.message("done"))};
    Process down(2, "down", std::move(downProgram));
    down.setState(ProcessState::Running);
    down.step(0, 1);
    check(down.variable("y") == 0, "SUBTRACT clamps at 0");
}

void testAutoDeclareAndPrintVar() {
    Program program;
    const VarId z = program.var("z");
    const VarId undeclared = program.var("undeclared");
    program.code = {Instruction::add(z, Operand::variable(undeclared), Operand::literal(7)),
                    Instruction::print(program.message("Value from: "), z)};
    Process p(1, "p", std::move(program));
    runToEnd(p);
    const auto logs = p.formattedLogs();
    check(logs.size() == 1 && endsWith(logs[0], "Core:0 \"Value from: 7\""),
          "PRINT appends variable value");
}

void testNestedForCounts() {
    // 3 * (1 + 2 * (2 * 1)) = 15 leaves.
    Program program;
    auto inner = Instruction::loop({Instruction::print(program.message("hi"))}, 2);
    auto middle = Instruction::loop({inner}, 2);
    auto outer = Instruction::loop({Instruction::declare(program.var("a"), 1), middle}, 3);
    program.code = {outer};
    Process p(1, "loops", std::move(program));
    check(p.totalInstructions() == 15, "nested FOR expanded count is 15");
    runToEnd(p);
    check(p.executedInstructions() == 15, "nested FOR executes 15 leaves");
    check(p.formattedLogs().size() == 12, "nested FOR prints 12 times");
}

void testSleep() {
    Program program;
    program.code = {Instruction::sleep(3), Instruction::print(program.message("awake"))};
    Process p(1, "sleepy", std::move(program));
    p.setState(ProcessState::Running);
    p.step(0, 10);
    check(p.state() == ProcessState::Sleeping, "SLEEP puts process to sleep");
    check(p.wakeTick() == 14, "SLEEP(3) at tick 10 wakes at tick 14");
    check(p.executedInstructions() == 1, "SLEEP counts as an instruction");
}

void testGeneratorBounds() {
    const std::vector<std::pair<uint64_t, uint64_t>> ranges = {
        {1, 1}, {1, 5}, {1000, 2000}, {50000, 60000}, {1ULL << 32, 1ULL << 32}};
    for (const auto& [lo, hi] : ranges) {
        for (int trial = 0; trial < 5; ++trial) {
            const uint64_t count = InstructionGenerator::generate("p01", lo, hi).expandedCount();
            check(count >= lo && count <= hi,
                  "generated length " + std::to_string(count) + " within [" + std::to_string(lo) +
                      ", " + std::to_string(hi) + "]");
        }
    }

    Process p(1, "p01", InstructionGenerator::generate("p01", 1000, 2000));
    runToEnd(p);
    check(p.executedInstructions() == p.totalInstructions(), "generated program runs to total");
    for (const auto& line : p.formattedLogs()) {
        check(endsWith(line, "\"Hello world from p01!\""), "default PRINT message");
        break;
    }
}

void testConfig() {
    const std::string path = "test-config.txt";
    auto load = [&](const std::string& text, Config& cfg) {
        std::ofstream(path) << text;
        std::string error;
        std::vector<std::string> warnings;
        const bool ok = Config::load(path, cfg, error, warnings);
        std::remove(path.c_str());
        return ok;
    };

    Config cfg;
    check(load("num-cpu 4\nscheduler \"fcfs\"\nquantum-cycles 5\nbatch-process-freq 1\n"
               "min-ins 1000\nmax-ins 2000\ndelays-per-exec 3\n",
               cfg),
          "valid config loads");
    check(cfg.scheduler == "fcfs" && cfg.delaysPerExec == 3, "quoted scheduler and delays-per-exec");

    check(!load("num-cpu 0\n", cfg), "num-cpu 0 rejected");
    check(!load("num-cpu 129\n", cfg), "num-cpu 129 rejected");
    check(!load("min-ins 10\nmax-ins 5\n", cfg), "min-ins > max-ins rejected");
    check(!load("scheduler \"sjf\"\n", cfg), "unknown scheduler rejected");
    check(!load("max-ins -1\n", cfg), "negative value rejected");
    check(load("max-ins 4294967296\n", cfg) && cfg.maxIns == (1ULL << 32), "2^32 accepted");
}

}  // namespace

int main() {
    testClamping();
    testAutoDeclareAndPrintVar();
    testNestedForCounts();
    testSleep();
    testGeneratorBounds();
    testConfig();

    if (failures == 0) std::cout << "All tests passed.\n";
    return failures == 0 ? 0 : 1;
}
