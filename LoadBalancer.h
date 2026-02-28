/**
 * @file LoadBalancer.h
 * @brief Defines the LoadBalancer class and SimulationStats struct used to
 *        drive and report on the entire load balancer simulation.
 *
 * The LoadBalancer owns a pool of WebServer objects, a FIFO request queue,
 * an IP firewall (IPBlocker), and an output log file. Each call to run()
 * executes the full simulation and returns a SimulationStats summary.
 *
 * @author Karan Bhagat
 * @date 2026
 */

#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <fstream>
#include <queue>
#include <string>
#include <vector>

#include "Config.h"
#include "IPBlocker.h"
#include "Request.h"
#include "WebServer.h"

/**
 * @struct SimulationStats
 * @brief Aggregated counters collected during a simulation run.
 *
 * An instance of this struct is returned by LoadBalancer::run() and is
 * used both for in-log summaries and for the terminal report printed by
 * main().
 */
struct SimulationStats {
    int generatedRequests;  ///< Total requests created (includes blocked ones).
    int acceptedRequests;   ///< Requests that passed the firewall and entered the queue.
    int blockedRequests;    ///< Requests rejected by the IPBlocker firewall.
    int completedRequests;  ///< Requests that finished processing on a server.
    int addedServers;       ///< Number of scale-up events (servers added).
    int removedServers;     ///< Number of scale-down events (servers removed).
    int peakQueueSize;      ///< Largest queue depth observed across all cycles.
    int finalQueueSize;     ///< Queue depth at the end of the last cycle.
    int finalServerCount;   ///< Number of active servers when the simulation ended.

    SimulationStats() {
        generatedRequests = 0;
        acceptedRequests = 0;
        blockedRequests = 0;
        completedRequests = 0;
        addedServers = 0;
        removedServers = 0;
        peakQueueSize = 0;
        finalQueueSize = 0;
        finalServerCount = 0;
    }
};

/**
 * @class LoadBalancer
 * @brief Core simulation class that manages the server pool, request queue,
 *        auto-scaling logic, firewall checks, and log output.
 *
 * On construction the LoadBalancer receives a Config struct (settings) and
 * an IPBlocker (firewall). Calling run() initializes the server pool, fills
 * an initial queue, then steps through every simulation cycle. At each cycle
 * the balancer:
 *  -# Optionally generates new incoming requests via randomAddNewRequests().
 *  -# Dispatches queued requests to idle servers and ticks all busy servers
 *     (processTick()).
 *  -# Evaluates whether to scale up or scale down the server pool
 *     (balanceLoad()).
 *
 * All notable events are written to the log file with color-coded tags.
 */
class LoadBalancer {
public:
    /**
     * @brief Constructs the LoadBalancer and opens the log file.
     * @param config  Simulation settings (server count, cycle count, etc.).
     * @param blocker Firewall object used to validate incoming request IPs.
     */
    LoadBalancer(const Config& config, const IPBlocker& blocker);

    /**
     * @brief Destructor. Frees all dynamically allocated WebServer objects
     *        and closes the log file.
     */
    ~LoadBalancer();

    /**
     * @brief Attempts to enqueue an incoming request.
     *
     * The source IP is checked against the IPBlocker. Blocked requests are
     * logged with a @c [BLOCK] tag and counted in stats. Accepted requests
     * are pushed onto the FIFO queue.
     *
     * @param request The Request to enqueue.
     */
    void addRequest(const Request& request);

    /**
     * @brief Allocates a new WebServer and appends it to the server pool.
     */
    void addServer();

    /**
     * @brief Removes an idle server from the pool to free capacity.
     * @return @c true if an idle server was found and removed;
     *         @c false if all servers are currently busy.
     */
    bool removeServer();

    /**
     * @brief Evaluates queue depth and adjusts the server pool size.
     *
     * Uses per-server thresholds (hard-coded as local constants):
     * - Scale up  when queue depth exceeds MAX_QUEUE_PER_SERVER × servers.
     * - Scale down when queue depth falls below MIN_QUEUE_PER_SERVER × servers
     *   AND the scaling cooldown timer has expired.
     */
    void balanceLoad();

    /**
     * @brief Executes one simulation clock cycle.
     *
     * Iterates over all servers: idle servers receive the next queued
     * request (if any); busy servers are ticked and their completion
     * counter is updated when they finish.
     */
    void processTick();

    /**
     * @brief Runs the complete simulation from start to finish.
     *
     * Initializes the server pool and request queue, then iterates for
     * Config::simulationCycles cycles. On completion writes a summary
     * block to the log file.
     *
     * @return A SimulationStats struct containing final counters for the run.
     */
    SimulationStats run();

private:
    Config config;                      ///< Copy of the simulation configuration.
    IPBlocker* ipBlocker;               ///< Pointer to the firewall/IP blocker.
    std::ofstream logFile;              ///< Output stream for the simulation log.
    std::queue<Request> requestQueue;   ///< FIFO queue of pending requests.
    std::vector<WebServer*> servers;    ///< Pool of dynamically allocated servers.

    int currentTime;      ///< Current simulation cycle number (1-based).
    int nextRequestId;    ///< Auto-incrementing ID counter for new requests.
    int cooldownTimer;    ///< Cycles remaining before the next scale-down is allowed.
    SimulationStats stats;///< Accumulates counters as the simulation runs.

    /**
     * @brief Creates a new randomised Request using the current ID counter.
     * @return A fully populated Request ready for queuing.
     */
    Request generateRequest();

    /** @brief Creates Config::initialServers WebServer objects at simulation start. */
    void initializeServers();

    /**
     * @brief Pre-fills the request queue before the main loop begins.
     *
     * Target depth is initialServers * initialQueueMultiplier. Requests are
     * generated until that depth is reached or the firewall rejects them.
     */
    void fillInitialQueue();

    /**
     * @brief Randomly injects new requests during the main simulation loop.
     *
     * Called once per cycle. Uses rand() to decide whether 0, 1, or 2 new
     * requests should arrive this cycle.
     */
    void randomAddNewRequests();

    /**
     * @brief Core log-write helper used by all logging methods.
     * @param level     Tag string (e.g. "INFO", "BLOCK", "SCALE UP").
     * @param colorCode ANSI escape code for terminal color (unused in file output).
     * @param message   Log message body.
     */
    void writeLog(const std::string& level, const std::string& colorCode, const std::string& message);

    /**
     * @brief Writes an informational message to the log file with the [INFO] tag.
     * @param message Text to write.
     */
    void logInfo(const std::string& message);
};

#endif