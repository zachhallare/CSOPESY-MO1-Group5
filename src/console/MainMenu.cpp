#include "console/MainMenu.h"

#include <iostream>

#include "console/ConsoleManager.h"
#include "report/Report.h"
#include "util/StringUtil.h"
#include "util/TimeUtil.h"

namespace {

const char* kLogPath = "csopesy-log.txt";

const char* kAsciiArt = R"ART(
  _____  _____  ____  _____  ______  _______     __
 / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
| |    | (___ | |  | | |__) | |__  | (___  \ \_/ /
| |     \___ \| |  | |  ___/|  __|  \___ \  \   /
| |____ ____) | |__| | |    | |____ ____) |  | |
 \_____|_____/ \____/|_|    |______|_____/   |_|
)ART";

}  // namespace

void MainMenu::printBanner() {
    const std::string divider(52, '-');
    std::cout << kAsciiArt << divider << "\n"
              << "Welcome to CSOPESY Emulator!\n\n"
              << "Developers:\n"
              << "Hallare, Zach\n"
              << "Ramos, Marga\n"
              << "Tan, Neil\n\n"
              << "Last updated: " << util::buildDate() << "\n"
              << divider << "\n\n";
}

void MainMenu::handle(const std::string& line) {
    const auto args = util::tokenize(line);
    const std::string command = util::toLower(args[0]);

    if (command == "exit") {
        console_.requestExit();
        return;
    }
    if (command == "initialize") {
        console_.initialize();
        return;
    }
    if (!console_.isInitialized()) {
        std::cout << "Please run 'initialize' first.\n";
        return;
    }

    if (command == "screen") {
        handleScreen(args);
    } else if (command == "screen-ls") {
        listProcesses();
    } else if (command == "scheduler-start" || command == "scheduler-test") {
        std::cout << (console_.scheduler().startGenerating()
                          ? "Scheduler started generating processes.\n"
                          : "Scheduler is already generating processes.\n");
    } else if (command == "scheduler-stop") {
        std::cout << (console_.scheduler().stopGenerating()
                          ? "Scheduler stopped generating processes.\n"
                          : "Scheduler is not generating processes.\n");
    } else if (command == "report-util") {
        reportUtil();
    } else if (command == "clear" || command == "cls") {
        ConsoleManager::clearScreen();
        printBanner();
    } else {
        std::cout << "Command not recognized.\n";
    }
}

void MainMenu::handleScreen(const std::vector<std::string>& args) {
    const std::string flag = args.size() > 1 ? args[1] : "";

    if (flag == "-ls") {
        listProcesses();
        return;
    }
    if ((flag != "-s" && flag != "-r") || args.size() < 3) {
        std::cout << "Usage: screen -s <process name> | screen -r <process name> | screen -ls\n";
        return;
    }

    const std::string& name = args[2];
    if (flag == "-s") {
        auto process = console_.scheduler().createProcess(name);
        if (!process) {
            std::cout << "Process " << name << " already exists.\n";
            return;
        }
        console_.attach(std::move(process));
        return;
    }

    auto process = console_.scheduler().findProcess(name);
    if (!process || process->isFinished()) {
        std::cout << "Process " << name << " not found.\n";
        return;
    }
    console_.attach(std::move(process));
}

void MainMenu::listProcesses() {
    std::cout << report::build(console_.scheduler().snapshot()) << "\n";
}

void MainMenu::reportUtil() {
    std::string absolutePath;
    if (!report::writeToFile(report::build(console_.scheduler().snapshot()), kLogPath, absolutePath)) {
        std::cout << "Failed to write " << kLogPath << ".\n";
        return;
    }
    std::cout << "Report generated at " << absolutePath << "!\n";
}
