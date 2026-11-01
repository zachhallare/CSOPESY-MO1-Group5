#pragma once

#include <memory>
#include <string>

#include "config/Config.h"
#include "console/MainMenu.h"
#include "console/ProcessScreen.h"
#include "process/Process.h"
#include "scheduler/Scheduler.h"

// Owns the REPL loop and switches between the main menu and an attached process screen.
class ConsoleManager {
public:
    ConsoleManager();
    ~ConsoleManager();

    void run();

    // Loads config.txt and starts the scheduler. Prints the outcome.
    void initialize();
    bool isInitialized() const { return scheduler_ != nullptr; }
    Scheduler& scheduler() { return *scheduler_; }

    void attach(std::shared_ptr<Process> process);
    void requestExit() { running_ = false; }

    static void clearScreen();

private:
    void detach();

    std::unique_ptr<Scheduler> scheduler_;
    MainMenu mainMenu_;
    std::unique_ptr<ProcessScreen> attached_;  // null while on the main menu
    bool running_ = true;
};
