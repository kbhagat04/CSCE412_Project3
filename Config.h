// Config.h
// holds all the settings for the simulation and reads them from a file

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

/**
 * @struct Config
 * @brief All the settings the simulation needs to run.
 *        Defaults are set here, can be overridden by the config file.
 */
struct Config {
    int initialServers = 10;
    int simulationCycles = 10000;
    int initialQueueMultiplier = 100;

    int scalingCooldownCycles = 25;

    int minRequestTime = 1;
    int maxRequestTime = 15;


    int statusPrintInterval = 500;
    std::string logFilePath = "load_balancer.log";
    unsigned int seed = 0;

    std::vector<std::string> blockedRanges;
};

/**
 * @class ConfigLoader
 * @brief Reads a config file and fills in a Config struct.
 */
class ConfigLoader {
public:
    /**
     * @brief Opens the config file and parses each key=value line.
     * @param path Path to the config file.
     * @param config Config struct to fill in.
     * @return false if file couldn't be opened.
     */
    static bool loadFromFile(const std::string& path, Config& config);
};

#endif