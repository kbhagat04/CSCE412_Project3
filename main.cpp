// main.cpp
// sets up the simulation from config/user input and runs it

#include <iostream>
#include <string>
#include "Config.h"
#include "IPBlocker.h"
#include "LoadBalancer.h"

// asks the user for an int, keeps the current value if they just hit enter
void promptForInt(const std::string& prompt, int& value) {
    std::cout << prompt << " [" << value << "]: ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            value = std::stoi(input);
        } catch (...) {
            std::cout << "Invalid input, keeping " << value << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::string configPath = "config.txt";
    if (argc > 1) {
        configPath = argv[1];
    }

    Config config;
    ConfigLoader::loadFromFile(configPath, config);

    promptForInt("Enter number of initial servers", config.initialServers);
    promptForInt("Enter simulation time in clock cycles", config.simulationCycles);

    IPBlocker blocker;
    for (const auto& range : config.blockedRanges) {
        if (!blocker.addBlockedRange(range)) {
            std::cerr << "[WARN] Invalid blocked range ignored: " << range << '\n';
        }
    }

    std::cout << "[INFO] Config loaded from: " << configPath << '\n';
    std::cout << "[INFO] Blocked ranges loaded: " << config.blockedRanges.size() << '\n';

    LoadBalancer balancer(config, blocker);
    SimulationStats stats = balancer.run();

    std::cout << "[INFO] ==== Simulation Summary ====\n";
    std::cout << "[INFO] Generated requests: " << stats.generatedRequests << '\n';
    std::cout << "[INFO] Accepted requests: " << stats.acceptedRequests << '\n';
    std::cout << "[INFO] Blocked requests: " << stats.blockedRequests << '\n';
    std::cout << "[INFO] Completed requests: " << stats.completedRequests << '\n';
    std::cout << "[INFO] Peak queue size: " << stats.peakQueueSize << '\n';
    std::cout << "[INFO] Final queue size: " << stats.finalQueueSize << '\n';
    std::cout << "[INFO] Servers added: " << stats.addedServers << '\n';
    std::cout << "[INFO] Servers removed: " << stats.removedServers << '\n';
    std::cout << "[INFO] Final server count: " << stats.finalServerCount << '\n';
    std::cout << "[INFO] Log file: " << config.logFilePath << '\n';

    return 0;
}