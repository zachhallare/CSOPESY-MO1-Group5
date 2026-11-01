#include "report/Report.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace report {

namespace {

const std::string kDivider(38, '-');

std::string progress(const Process& process) {
    return std::to_string(process.executedInstructions()) + " / " +
           std::to_string(process.totalInstructions());
}

}  // namespace

std::string build(const Scheduler::Snapshot& snapshot) {
    const size_t used = snapshot.running.size();
    const size_t utilization = snapshot.totalCores == 0 ? 0 : used * 100 / snapshot.totalCores;

    size_t nameWidth = 10;
    for (const auto& [core, process] : snapshot.running) nameWidth = std::max(nameWidth, process->name().size());
    for (const auto& process : snapshot.finished) nameWidth = std::max(nameWidth, process->name().size());
    nameWidth += 2;

    std::ostringstream out;
    out << "CPU utilization: " << utilization << "%\n"
        << "Cores used: " << used << "\n"
        << "Cores available: " << snapshot.totalCores - used << "\n\n"
        << kDivider << "\n"
        << "Running processes:\n";
    for (const auto& [core, process] : snapshot.running) {
        out << std::left << std::setw(static_cast<int>(nameWidth)) << process->name() << "("
            << process->createdAt() << ")    Core: " << std::setw(6) << core << progress(*process)
            << "\n";
    }

    out << "\nFinished processes:\n";
    for (const auto& process : snapshot.finished) {
        out << std::left << std::setw(static_cast<int>(nameWidth)) << process->name() << "("
            << process->createdAt() << ")    Finished    " << progress(*process) << "\n";
    }
    out << kDivider << "\n";
    return out.str();
}

bool writeToFile(const std::string& text, const std::string& path, std::string& absolutePath) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;
    file << text;
    if (!file) return false;
    absolutePath = std::filesystem::absolute(path).generic_string();
    return true;
}

}  // namespace report
