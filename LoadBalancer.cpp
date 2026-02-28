// LoadBalancer.cpp

#include "LoadBalancer.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

// ANSI color codes for terminal output
#define RESET  "\033[0m"
#define CYAN   "\033[36m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RED    "\033[31m"

const int MIN_QUEUE_PER_SERVER = 50;
const int MAX_QUEUE_PER_SERVER = 80;

// constructor - copy config, set up blocker, open log, seed RNG
LoadBalancer::LoadBalancer(const Config& cfg, const IPBlocker& blocker) {
    config = cfg;
    ipBlocker = new IPBlocker(blocker);
    logFile.open(config.logFilePath);
    currentTime = 0;
    nextRequestId = 1;
    cooldownTimer = 0;
    if (config.seed == 0) {
        srand((unsigned int)time(nullptr));
    } else {
        srand(config.seed);
    }
}

// destructor - free servers, blocker, close log
LoadBalancer::~LoadBalancer() {
    for (int i = 0; i < (int)servers.size(); i++) {
        delete servers[i];
    }
    servers.clear();

    delete ipBlocker;
    ipBlocker = nullptr;

    if (logFile.is_open()) {
        logFile.close();
    }
}

// make a new random request with the next available ID
Request LoadBalancer::generateRequest() {
    return Request::randomRequest(nextRequestId++, config.minRequestTime, config.maxRequestTime);
}

// checks if request IP is blocked, otherwise pushes it onto the queue
void LoadBalancer::addRequest(const Request& request) {
    stats.generatedRequests++;
    if (ipBlocker->isBlocked(request.ipIn)) {
        stats.blockedRequests++;
        std::string blockMsg = "Request #" + std::to_string(request.id) + " BLOCKED | src=" + request.ipIn + " dst=" + request.ipOut;
        writeLog("BLOCK", YELLOW, blockMsg);
        return;
    }

    requestQueue.push(request);
    stats.acceptedRequests++;
    if (logFile.is_open()) {
        logFile << "[QUEUED] Request #" << request.id << " | " << request.ipIn << " -> " << request.ipOut << " | type=" << request.jobType << " time=" << request.timeRequired << '\n';
    }
}

// fill the queue before the simulation starts (servers * multiplier)
void LoadBalancer::fillInitialQueue() {
    int targetQueueSize = config.initialServers * config.initialQueueMultiplier;
    while ((int)requestQueue.size() < targetQueueSize) {
        addRequest(generateRequest());
    }

    stats.peakQueueSize = (int)requestQueue.size();
}

// randomly add 0 or 1 new requests each cycle
void LoadBalancer::randomAddNewRequests() {
    if (rand() % 2 == 0) {
        addRequest(generateRequest());
    }
}

// create the starting pool of servers
void LoadBalancer::initializeServers() {
    if (config.initialServers < 1) {
        config.initialServers = 1;
    }

    for (int index = 0; index < config.initialServers; index++) {
        addServer();
    }
}

// create a new web server and add it to the pool
void LoadBalancer::addServer() {
    std::string id = std::to_string(servers.size() + 1);
    servers.push_back(new WebServer(id));
}

// find an available server to remove (search from back to front)
bool LoadBalancer::removeServer() {
    for (int i = (int)servers.size() - 1; i >= 0; i--) {
        if (servers[i]->isAvailable()) {
            WebServer* target = servers[i];
            servers.erase(servers.begin() + i);
            delete target;
            return true;
        }
    }
    return false;
}

// check queue vs thresholds and add/remove servers if needed
void LoadBalancer::balanceLoad() {
    if (cooldownTimer > 0) {
        cooldownTimer--;
        return;
    }

    int serverCount = (int)servers.size();
    int queueSize = (int)requestQueue.size();
    
    int lowerThreshold = MIN_QUEUE_PER_SERVER * serverCount;
    int upperThreshold = MAX_QUEUE_PER_SERVER * serverCount;

    if (queueSize > upperThreshold) {
        addServer();
        stats.addedServers++;
        cooldownTimer = config.scalingCooldownCycles;
        std::string scaleMsg = "Cycle " + std::to_string(currentTime) + ": queue=" + std::to_string(queueSize) + " exceeded max threshold=" + std::to_string(upperThreshold) + ", added 1 server (now " + std::to_string(servers.size()) + ")";
        writeLog("SCALE UP", GREEN, scaleMsg);
    } else if (queueSize < lowerThreshold && serverCount > 1) {
        if (removeServer()) {
            stats.removedServers++;
            cooldownTimer = config.scalingCooldownCycles;
            std::string scaleMsg = "Cycle " + std::to_string(currentTime) + ": queue=" + std::to_string(queueSize) + " below min threshold=" + std::to_string(lowerThreshold) + ", removed 1 server (now " + std::to_string(servers.size()) + ")";
            writeLog("SCALE DOWN", RED, scaleMsg);
        }
    }
}

// one clock cycle: give idle servers work, then tick all busy servers
void LoadBalancer::processTick() {
    for (int i = 0; i < (int)servers.size(); i++) {
        if (requestQueue.empty()) {
            break;
        }

        if (servers[i]->isAvailable()) {
            Request next = requestQueue.front();
            requestQueue.pop();
            // file-only dispatch log
            if (logFile.is_open()) {
                logFile << "[ASSIGNED] Request #" << next.id << " -> server " << servers[i]->id() << " | " << next.ipIn << " -> " << next.ipOut << " | time=" << next.timeRequired << '\n';
            }
            servers[i]->processRequest(&next);
        }
    }

    for (int i = 0; i < (int)servers.size(); i++) {
        if (servers[i]->processTick()) {
            stats.completedRequests++;
        }
    }
}

// writes a tagged message to both terminal (with color) and log file
void LoadBalancer::writeLog(const std::string& level, const std::string& colorCode, const std::string& message) {
    std::string formatted = "[" + level + "] " + message;
    std::cout << colorCode << formatted << RESET << '\n';
    if (logFile.is_open()) {
        logFile << formatted << '\n';
    }
}

// shortcut to write an INFO-level log line
void LoadBalancer::logInfo(const std::string& message) {
    writeLog("INFO", CYAN, message);
}

// runs the full simulation loop and returns stats at the end
SimulationStats LoadBalancer::run() {
    initializeServers();

    std::string bannerMsg = "Starting simulation for " + std::to_string(config.simulationCycles) + " cycles with " + std::to_string(servers.size()) + " server(s)";
    logInfo(bannerMsg);

    if (!config.blockedRanges.empty()) {
        std::string rangeMsg = "Blocked IP ranges (" + std::to_string(config.blockedRanges.size()) + "): ";
        for (int i = 0; i < (int)config.blockedRanges.size(); i++) {
            if (i > 0) {
                rangeMsg += ", ";
            }
            rangeMsg += config.blockedRanges[i];
        }
        logInfo(rangeMsg);
    }

    fillInitialQueue();

    std::string qinfoMsg = "Initial queue: " + std::to_string(requestQueue.size()) + " requests | generated=" + std::to_string(stats.generatedRequests) + " | blocked=" + std::to_string(stats.blockedRequests) + " | accepted=" + std::to_string(stats.acceptedRequests);
    logInfo(qinfoMsg);

    int cap = (int)servers.size() * MAX_QUEUE_PER_SERVER;
    int fillPct = cap > 0 ? (int)(requestQueue.size() * 100 / cap) : 0;
    std::string capinfoMsg = "Queue capacity: " + std::to_string(cap) + " (" + std::to_string(MAX_QUEUE_PER_SERVER) + " per server) | fill=" + std::to_string(fillPct) + "%  [scale-up >" + std::to_string(MAX_QUEUE_PER_SERVER) + "/srv, scale-down <" + std::to_string(MIN_QUEUE_PER_SERVER) + "/srv]";
    logInfo(capinfoMsg);

    for (int cycle = 1; cycle <= config.simulationCycles; cycle++) {
        currentTime = cycle;
        randomAddNewRequests();
        processTick();

        if ((int)requestQueue.size() > stats.peakQueueSize) {
            stats.peakQueueSize = (int)requestQueue.size();
        }

        balanceLoad();

        if (config.statusPrintInterval > 0 && cycle % config.statusPrintInterval == 0) {
            int capacity = (int)servers.size() * MAX_QUEUE_PER_SERVER;
            int qsize = (int)requestQueue.size();
            int pct = capacity > 0 ? qsize * 100 / capacity : 0;
            std::string statusMsg = "Cycle " + std::to_string(cycle) + "/" + std::to_string(config.simulationCycles) + "  |  queue " + std::to_string(qsize) + "/" + std::to_string(capacity) + " (" + std::to_string(pct) + "%)  |  servers=" + std::to_string(servers.size()) + "  |  gen=" + std::to_string(stats.generatedRequests) + " blocked=" + std::to_string(stats.blockedRequests) + " done=" + std::to_string(stats.completedRequests);
            logInfo(statusMsg);
        }
    }

    stats.finalQueueSize = (int)requestQueue.size();
    stats.finalServerCount = (int)servers.size();

    if (logFile.is_open()) {
        logFile << '\n';
        logFile << "[INFO] ==== Simulation Summary ====\n";
        logFile << "[INFO] Generated requests : " << stats.generatedRequests << '\n';
        logFile << "[INFO] Accepted requests  : " << stats.acceptedRequests << '\n';
        logFile << "[INFO] Blocked requests   : " << stats.blockedRequests << '\n';
        logFile << "[INFO] Completed requests : " << stats.completedRequests << '\n';
        logFile << "[INFO] Peak queue size    : " << stats.peakQueueSize << '\n';
        logFile << "[INFO] Final queue size   : " << stats.finalQueueSize << '\n';
        logFile << "[INFO] Servers added      : " << stats.addedServers << '\n';
        logFile << "[INFO] Servers removed    : " << stats.removedServers << '\n';
        logFile << "[INFO] Final server count : " << stats.finalServerCount << '\n';
        logFile << "[INFO] Log file           : " << config.logFilePath << '\n';
    }

    return stats;
}