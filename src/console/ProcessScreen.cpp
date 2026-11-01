#include "console/ProcessScreen.h"

#include <iostream>
#include <sstream>

#include "util/StringUtil.h"

void ProcessScreen::show() const {
    // Read the finished flag before the logs so a "Finished!" view is never missing lines.
    const bool finished = process_->isFinished();
    const auto logs = process_->formattedLogs();

    std::ostringstream out;
    out << "Process name: " << process_->name() << "\n"
        << "ID: " << process_->id() << "\n"
        << "Logs:\n";
    for (const auto& line : logs) out << line << "\n";
    out << "\n";
    if (finished) {
        out << "Finished!\n";
    } else {
        out << "Current instruction line: " << process_->executedInstructions() << "\n"
            << "Lines of code: " << process_->totalInstructions() << "\n";
    }
    std::cout << out.str() << "\n";
}

bool ProcessScreen::handle(const std::string& line) const {
    const std::string command = util::toLower(util::trim(line));
    if (command == "exit") return false;
    if (command == "process-smi") {
        show();
    } else {
        std::cout << "Command not recognized.\n";
    }
    return true;
}
