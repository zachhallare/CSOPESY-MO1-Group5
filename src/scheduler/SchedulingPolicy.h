#pragma once

#include <string>

enum class SchedulingPolicy { FCFS, RoundRobin };

// Expects a lower-case name already validated by Config ("fcfs" or "rr").
inline SchedulingPolicy parseSchedulingPolicy(const std::string& name) {
    return name == "fcfs" ? SchedulingPolicy::FCFS : SchedulingPolicy::RoundRobin;
}
