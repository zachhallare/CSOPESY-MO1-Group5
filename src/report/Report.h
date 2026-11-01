#pragma once

#include <string>

#include "scheduler/Scheduler.h"

namespace report {

// CPU utilization summary plus running and finished processes, shared by
// "screen -ls" and "report-util".
std::string build(const Scheduler::Snapshot& snapshot);

// Writes `text` to `path`, overwriting it. On success, fills `absolutePath`.
bool writeToFile(const std::string& text, const std::string& path, std::string& absolutePath);

}  // namespace report
