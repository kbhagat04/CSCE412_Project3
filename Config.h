/**
 * @file Config.h
 * @brief Defines the Config struct and ConfigLoader class used to
 *        configure the load balancer simulation.
 *
 * @author Karan Bhagat
 * @date 2026
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

/**
 * @struct Config
 * @brief All tunable parameters for a simulation run.
 *
 * Sensible defaults are set in the constructor. Any field can be
 * overridden by a @c key=value line in the configuration file
 * (see ConfigLoader::loadFromFile()).
 */
struct Config {
    int initialServers;           ///< Number of servers created at startup. Default: 10.
    int simulationCycles;         ///< Total clock cycles to simulate. Default: 10000.
    int initialQueueMultiplier;   ///< Initial queue depth = initialServers * this. Default: 100.
    int scalingCooldownCycles;    ///< Minimum cycles between consecutive scale-down events. Default: 25.
    int minRequestTime;           ///< Shortest possible request processing time (cycles). Default: 1.
    int maxRequestTime;           ///< Longest possible request processing time (cycles). Default: 30.
    int statusPrintInterval;      ///< Log a status line every N cycles (0 = disabled). Default: 500.
    std::string logFilePath;      ///< Path to the output log file. Default: @c "load_balancer.log".
    unsigned int seed;            ///< RNG seed (0 = use time-based seed). Default: 0.
    std::vector<std::string> blockedRanges; ///< IP ranges/CIDRs to block, loaded from config file.

    /**
     * @brief Default constructor. Sets all fields to the documented defaults.
     */
    Config() {
        initialServers = 10;
        simulationCycles = 10000;
        initialQueueMultiplier = 100;
        scalingCooldownCycles = 25;
        minRequestTime = 1;
        maxRequestTime = 30;
        statusPrintInterval = 500;
        logFilePath = "load_balancer.log";
        seed = 0;
    }
};

/**
 * @class ConfigLoader
 * @brief Utility class that parses a plain-text configuration file and
 *        populates a Config struct.
 *
 * The configuration file uses simple @c key=value syntax, one setting
 * per line. Lines beginning with @c # are treated as comments and
 * ignored. Unrecognized keys are silently skipped so that the file
 * can contain notes without breaking the loader.
 */
class ConfigLoader {
public:
    /**
     * @brief Parses a configuration file and fills in the provided Config.
     *
     * Fields not present in the file retain the default values set by the
     * Config constructor. If the file cannot be opened the function returns
     * @c false and the Config is left unchanged.
     *
     * @param path   Path to the configuration file.
     * @param config Config struct to update with loaded values.
     * @return @c true on success; @c false if the file could not be opened.
     */
    static bool loadFromFile(const std::string& path, Config& config);
};

#endif