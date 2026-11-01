#pragma once

#include <memory>
#include <string>

#include "process/Process.h"

// The screen shown while attached to a process via "screen -s" or "screen -r".
class ProcessScreen {
public:
    explicit ProcessScreen(std::shared_ptr<Process> process) : process_(std::move(process)) {}

    // Prints name, ID, logs, and progress, or "Finished!" once the process is done.
    void show() const;

    // Returns false when the user leaves the screen.
    bool handle(const std::string& line) const;

private:
    std::shared_ptr<Process> process_;
};
