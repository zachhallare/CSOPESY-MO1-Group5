#include "console/ConsoleManager.h"

#include <cstdlib>
#include <iostream>
#include <vector>

#include "util/StringUtil.h"

namespace {

const char* kConfigPath = "config.txt";
const char* kPrompt = "root:\\> ";

}  // namespace

ConsoleManager::ConsoleManager() : mainMenu_(*this) {}

ConsoleManager::~ConsoleManager() {
    if (scheduler_) scheduler_->stop();
}

void ConsoleManager::run() {
    clearScreen();
    MainMenu::printBanner();

    std::string line;
    while (running_) {
        std::cout << kPrompt << std::flush;
        if (!std::getline(std::cin, line)) break;  // EOF
        line = util::trim(line);
        if (line.empty()) continue;

        if (attached_) {
            if (!attached_->handle(line)) detach();
        } else {
            mainMenu_.handle(line);
        }
    }
}

void ConsoleManager::initialize() {
    if (isInitialized()) {
        std::cout << "Already initialized.\n";
        return;
    }

    Config config;
    std::string error;
    std::vector<std::string> warnings;
    const bool ok = Config::load(kConfigPath, config, error, warnings);
    for (const auto& warning : warnings) std::cout << "Warning: " << warning << "\n";
    if (!ok) {
        std::cout << "Initialization failed: " << error << "\n";
        return;
    }

    scheduler_ = std::make_unique<Scheduler>(config);
    scheduler_->start();
    std::cout << "Initialized: " << config.numCpu << " CPU(s), scheduler " << config.scheduler
              << ", quantum " << config.quantumCycles << ", batch freq " << config.batchProcessFreq
              << ", instructions " << config.minIns << "-" << config.maxIns << ", delay "
              << config.delaysPerExec << ", pattern " << config.instructionPattern << ".\n";
}

void ConsoleManager::attach(std::shared_ptr<Process> process) {
    attached_ = std::make_unique<ProcessScreen>(std::move(process));
    clearScreen();
    attached_->show();
}

void ConsoleManager::detach() {
    attached_.reset();
    clearScreen();
    MainMenu::printBanner();
}

void ConsoleManager::clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\x1b[2J\x1b[H" << std::flush;
#endif
}
