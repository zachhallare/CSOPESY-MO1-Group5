#pragma once

#include <ctime>
#include <string>

namespace util {

// Local time as MM/DD/YYYY HH:MM:SSAM, the format used in logs and reports.
std::string formatTimestamp(std::time_t time);
std::string currentTimestamp();

// Date this binary was compiled, as MM-DD-YYYY, shown as "Last updated" in the banner.
std::string buildDate();

}  // namespace util
