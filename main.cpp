/**
 * @file main.cpp
 * @brief Entry point for the CSCE 412 Project 3 Load Balancer Simulation.
 *
 * @author Karan Bhagat
 * @date 2026
 */

/**
 * @mainpage Main Page
 *
 * @section intro Introduction
 * This project implements a dynamic web load balancer simulation in C++17.
 * A pool of WebServer instances processes an incoming stream of HTTP-style
 * requests over a configurable number of clock cycles. The server pool scales
 * up or down automatically based on queue depth, and an IP firewall
 * (IPBlocker) drops traffic from configured address ranges before it enters
 * the queue.
 *
 * @section classes Classes
 * - **LoadBalancer** – orchestrates the simulation; owns the server pool,
 *   request queue, scaling logic, and log output.
 * - **WebServer** – models one backend server; handles one request at a time
 *   and counts down its processing timer each clock cycle.
 * - **Request** – plain data struct representing one web request (source/dest
 *   IPs, job type, processing time).
 * - **IPBlocker** – firewall that rejects requests whose source IP falls
 *   inside a configured CIDR or dash-separated range.
 * - **Config / ConfigLoader** – holds all tunable parameters and parses them
 *   from a key=value configuration file.
 *
 * @section build Building and Running
 * @code
 *   make
 *   ./load_balancer_sim [config_file]
 * @endcode
 */

#include <iostream>
#include <cstdlib>
#include <string>
#include "Config.h"
#include "IPBlocker.h"
#include "LoadBalancer.h"

/**
 * @brief Prompts the user to enter a new integer value, keeping the
 *        existing value if the user presses Enter without typing anything.
 * @param prompt Display text shown before the current default in brackets.
 * @param value  Reference to the integer to update.
 */
void promptForInt(const std::string& prompt, int& value) {
    std::cout << prompt << " [" << value << "]: ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        int parsed = atoi(input.c_str());
        if (parsed > 0) {
            value = parsed;
        } else {
            std::cout << "Invalid input, keeping " << value << "\n";
        }
    }
}

/**
 * @brief Program entry point.
 * @param argc Argument count (1 = no extra args, 2 = config file path provided).
 * @param argv Argument vector; @c argv[1], if present, is the config file path.
 * @return 0 on normal exit.
 */
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
    for (int i = 0; i < (int)config.blockedRanges.size(); i++) {
        if (!blocker.addBlockedRange(config.blockedRanges[i])) {
            std::cerr << "[WARN] Invalid blocked range ignored: " << config.blockedRanges[i] << '\n';
        }
    }

    std::cout << "[INFO] Config loaded from: " << configPath << '\n' << "\n";

    LoadBalancer balancer(config, blocker);
    SimulationStats stats = balancer.run();

    std::cout << "\n==== Simulation Summary ====\n";
    std::cout << "Generated requests : " << stats.generatedRequests << '\n';
    std::cout << "Accepted requests  : " << stats.acceptedRequests << '\n';
    std::cout << "Blocked requests   : " << stats.blockedRequests << '\n';
    std::cout << "Completed requests : " << stats.completedRequests << '\n';
    std::cout << "Peak queue size    : " << stats.peakQueueSize << '\n';
    std::cout << "Final queue size   : " << stats.finalQueueSize << '\n';
    std::cout << "Servers added      : " << stats.addedServers << '\n';
    std::cout << "Servers removed    : " << stats.removedServers << '\n';
    std::cout << "Final server count : " << stats.finalServerCount << '\n';
    std::cout << "Log file           : " << config.logFilePath << '\n';

    return 0;
}