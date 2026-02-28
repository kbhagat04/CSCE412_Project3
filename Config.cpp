// Config.cpp

#include "Config.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cctype>

// strips leading/trailing whitespace
static std::string trim(const std::string& s) {
    int start = 0;
    int end = (int)s.size() - 1;
    while (start <= end && isspace(s[start])) start++;
    while (end >= start && isspace(s[end])) end--;
    return s.substr(start, end - start + 1);
}

// splits a comma separated list of IP ranges
static void parseBlockedRanges(const std::string& value, std::vector<std::string>& ranges) {
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) {
            ranges.push_back(item);
        }
    }
}

// reads the config file line by line and sets matching fields
bool ConfigLoader::loadFromFile(const std::string& path, Config& config) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        int eqPos = (int)line.find('=');
        if (eqPos == -1) {
            continue;
        }

        std::string key = trim(line.substr(0, eqPos));
        std::string val = trim(line.substr(eqPos + 1));

        if (key == "initial_servers") {
            config.initialServers = atoi(val.c_str());
        } else if (key == "simulation_cycles") {
            config.simulationCycles = atoi(val.c_str());
        } else if (key == "initial_queue_multiplier") {
            config.initialQueueMultiplier = atoi(val.c_str());
        } else if (key == "scaling_cooldown_cycles") {
            config.scalingCooldownCycles = atoi(val.c_str());
        } else if (key == "min_request_time") {
            config.minRequestTime = atoi(val.c_str());
        } else if (key == "max_request_time") {
            config.maxRequestTime = atoi(val.c_str());
        } else if (key == "status_print_interval") {
            config.statusPrintInterval = atoi(val.c_str());
        } else if (key == "log_file") {
            config.logFilePath = val;
        } else if (key == "seed") {
            config.seed = (unsigned int)atoi(val.c_str());
        } else if (key == "blocked_ranges") {
            config.blockedRanges.clear();
            parseBlockedRanges(val, config.blockedRanges);
        }
    }

    if (config.minRequestTime < 1) {
        config.minRequestTime = 1;
    }
    if (config.maxRequestTime < config.minRequestTime) {
        config.maxRequestTime = config.minRequestTime;
    }

    return true;
}