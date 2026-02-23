// LoadBalancer.h
// main class for the simulation - handles the server pool, request queue, scaling, and logging

#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <fstream>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "Config.h"
#include "IPBlocker.h"
#include "Request.h"
#include "WebServer.h"

/**
 * @struct SimulationStats
 * @brief Tracks numbers we care about after the simulation finishes.
 */
struct SimulationStats {
    int generatedRequests = 0;
    int acceptedRequests = 0;
    int blockedRequests = 0;
    int completedRequests = 0;
    int addedServers = 0;
    int removedServers = 0;
    int peakQueueSize = 0;
    int finalQueueSize = 0;
    int finalServerCount = 0;
};

/**
 * @class LoadBalancer
 * @brief Main class for the simulation. Manages servers, queue, scaling, and logging.
 */
class LoadBalancer {
public:
    /**
     * @brief Sets up the load balancer with config settings and a firewall blocker.
     * @param config Settings to use.
     * @param blocker IPBlocker to check incoming IPs against.
     */
    LoadBalancer(const Config& config, const IPBlocker& blocker);

    /**
     * @brief Destructor - cleans up server pointers and closes log file.
     */
    ~LoadBalancer();

    /**
     * @brief Adds a request to the queue (if not blocked by firewall).
     * @param request The request to add.
     */
    void addRequest(const Request& request);

    /**
     * @brief Runs one clock cycle - dispatches requests and ticks all servers.
     */
    void processTick();

    /**
     * @brief Checks queue size and adds/removes servers as needed.
     */
    void balanceLoad();

    /**
     * @brief Spins up a new WebServer and adds it to the pool.
     */
    void addServer();

    /**
     * @brief Removes an idle server to save capacity.
     * @return true if a server was removed.
     */
    bool removeServer();

    /**
     * @brief Logs an info message to terminal and log file.
     * @param event Message to log.
     */
    void logEvent(const std::string& event);

    /**
     * @brief Runs the whole simulation and returns final stats.
     * @return SimulationStats with totals for the run.
     */
    SimulationStats run();

private:
    Config config_;
    IPBlocker* ipBlocker_;
    std::ofstream logFile_;
    std::queue<Request> requestQueue_;
    std::vector<WebServer*> servers_;
    std::mt19937 generator_;

    int currentTime_ = 0;
    int nextRequestId_ = 1;
    int cooldownTimer_ = 0;
    SimulationStats stats_;

    void initializeServers();
    void fillInitialQueue();
    void maybeAddNewRequests();
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void writeLog(const std::string& level, const std::string& colorCode, const std::string& message);

    Request generateRequest();
};

#endif