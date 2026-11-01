#include "util/TimeUtil.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace util {

std::string formatTimestamp(std::time_t time) {
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%m/%d/%Y %I:%M:%S%p");
    return out.str();
}

std::string currentTimestamp() {
    return formatTimestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

std::string buildDate() {
    // __DATE__ looks like "Nov  1 2026".
    const std::string date = __DATE__;
    const std::string months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const int month = static_cast<int>(months.find(date.substr(0, 3))) / 3 + 1;
    const int day = std::stoi(date.substr(4, 2));

    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << month << '-' << std::setw(2) << day << '-'
        << date.substr(7, 4);
    return out.str();
}

}  // namespace util
