#pragma once

#include <string>
#include <vector>

class ConsoleManager;

// Commands available at the root prompt.
class MainMenu {
public:
    explicit MainMenu(ConsoleManager& console) : console_(console) {}

    static void printBanner();
    void handle(const std::string& line);

private:
    void handleScreen(const std::vector<std::string>& args);
    void listProcesses();
    void reportUtil();

    ConsoleManager& console_;
};
