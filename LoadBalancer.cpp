// LoadBalancer.cpp
// implementation of LoadBalancer - queue management, server scaling, logging

#include "LoadBalancer.h"
#include <algorithm>
#include <iostream>
#include <sstream>

// ANSI color codes for terminal output
#define RESET  "\033[0m"
#define CYAN   "\033[36m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RED    "\033[31m"

LoadBalancer::LoadBalancer(const Config& config, const IPBlocker& blocker)
    : config_(config), ipBlocker_(new IPBlocker(blocker)), logFile_(config.logFilePath) {
    if (config_.seed == 0) {
        std::random_device device;
        generator_ = std::mt19937(device());
    } else {
        generator_ = std::mt19937(config_.seed);
    }
}

LoadBalancer::~LoadBalancer() {
    for (WebServer* server : servers_) {
        delete server;
    }
    servers_.clear();

    delete ipBlocker_;
    ipBlocker_ = nullptr;

    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void LoadBalancer::logEvent(const std::string& event) {
    logInfo(event);
}

void LoadBalancer::logInfo(const std::string& message) {
    writeLog("INFO", CYAN, message);
}

void LoadBalancer::logWarning(const std::string& message) {
    writeLog("WARN", YELLOW, message);
}

void LoadBalancer::logError(const std::string& message) {
    writeLog("ERROR", RED, message);
}

void LoadBalancer::writeLog(const std::string& level, const std::string& colorCode, const std::string& message) {
    std::string formatted = "[" + level + "] " + message;
    std::cout << colorCode << formatted << RESET << '\n';
    if (logFile_.is_open()) {
        logFile_ << formatted << '\n';
    }
}

void LoadBalancer::addRequest(const Request& request) {
    stats_.generatedRequests++;
    if (ipBlocker_ != nullptr && ipBlocker_->isBlocked(request.ipIn)) {
        stats_.blockedRequests++;
        writeLog("BLOCK", YELLOW, "Request #" + std::to_string(request.id) +
                 " BLOCKED | src=" + request.ipIn +
                 " dst=" + request.ipOut);
        return;
    }

    requestQueue_.push(request);
    stats_.acceptedRequests++;
    // file-only: too noisy for terminal at high cycle counts
    if (logFile_.is_open()) {
        logFile_ << "[QUEUED] Request #" << request.id
                 << " | " << request.ipIn << " -> " << request.ipOut
                 << " | type=" << request.jobType
                 << " time=" << request.timeRequired << '\n';
    }
}

void LoadBalancer::processTick() {
    for (WebServer* server : servers_) {
        if (requestQueue_.empty()) {
            break;
        }

        if (server->isAvailable()) {
            Request next = requestQueue_.front();
            requestQueue_.pop();
            // file-only dispatch log
            if (logFile_.is_open()) {
                logFile_ << "[ASSIGNED] Request #" << next.id
                         << " -> server " << server->id()
                         << " | " << next.ipIn << " -> " << next.ipOut
                         << " | time=" << next.timeRequired << '\n';
            }
            server->processRequest(&next);
        }
    }

    for (WebServer* server : servers_) {
        if (server->processTick()) {
            stats_.completedRequests++;
        }
    }
}

void LoadBalancer::balanceLoad() {
    if (cooldownTimer_ > 0) {
        cooldownTimer_--;
        return;
    }

    int serverCount = (int)servers_.size();
    int queueSize = (int)requestQueue_.size();
    int lowerThreshold = config_.minQueuePerServer * serverCount;
    int upperThreshold = config_.maxQueuePerServer * serverCount;

    if (queueSize > upperThreshold) {
        addServer();
        stats_.addedServers++;
        cooldownTimer_ = config_.scalingCooldownCycles;
        writeLog("SCALE UP", GREEN, "Cycle " + std::to_string(currentTime_) +
                 ": queue above threshold, added 1 server (now " +
                 std::to_string(servers_.size()) + ")");
    } else if (queueSize < lowerThreshold && serverCount > 1) {
        if (removeServer()) {
            stats_.removedServers++;
            cooldownTimer_ = config_.scalingCooldownCycles;
            writeLog("SCALE DOWN", RED, "Cycle " + std::to_string(currentTime_) +
                     ": queue below threshold, removed 1 server (now " +
                     std::to_string(servers_.size()) + ")");
        }
    }
}

void LoadBalancer::addServer() {
    std::string id = std::to_string(servers_.size() + 1);
    servers_.push_back(new WebServer(id));
}

bool LoadBalancer::removeServer() {
    auto it = std::find_if(servers_.rbegin(), servers_.rend(),
                            [](WebServer* s) { return s->isAvailable(); });
    if (it == servers_.rend()) return false;

    WebServer* target = *it;
    servers_.erase(std::next(it).base());
    delete target;
    return true;
}

SimulationStats LoadBalancer::run() {
    initializeServers();

    // print banner BEFORE filling the queue so blocked warnings don't appear above it
    logEvent("Starting simulation for " + std::to_string(config_.simulationCycles) +
             " cycles with " + std::to_string(servers_.size()) + " server(s)");

    if (!config_.blockedRanges.empty()) {
        std::ostringstream ranges;
        ranges << "Blocked IP ranges (" << config_.blockedRanges.size() << "): ";
        for (int i = 0; i < (int)config_.blockedRanges.size(); i++) {
            if (i > 0) ranges << ", ";
            ranges << config_.blockedRanges[i];
        }
        logInfo(ranges.str());
    }

    fillInitialQueue();

    logInfo("Initial queue: " + std::to_string(requestQueue_.size()) + " requests"
            + " | generated=" + std::to_string(stats_.generatedRequests)
            + " | blocked=" + std::to_string(stats_.blockedRequests)
            + " | accepted=" + std::to_string(stats_.acceptedRequests));

    // show queue capacity context so it's easy to see how full things are
    int cap = (int)servers_.size() * config_.maxQueuePerServer;
    int fillPct = cap > 0 ? (int)(requestQueue_.size() * 100 / cap) : 0;
    logInfo("Queue capacity: " + std::to_string(cap)
            + " (" + std::to_string(config_.maxQueuePerServer) + " per server)"
            + " | fill=" + std::to_string(fillPct) + "%"
            + "  [scale-up >" + std::to_string(config_.maxQueuePerServer)
            + "/srv, scale-down <" + std::to_string(config_.minQueuePerServer) + "/srv]");
    logInfo("--------------------------------------------------");

    for (int cycle = 1; cycle <= config_.simulationCycles; ++cycle) {
        currentTime_ = cycle;
        maybeAddNewRequests();
        processTick();

        if ((int)requestQueue_.size() > stats_.peakQueueSize)
            stats_.peakQueueSize = (int)requestQueue_.size();

        balanceLoad();

        if (config_.statusPrintInterval > 0 && cycle % config_.statusPrintInterval == 0) {
            int capacity = (int)servers_.size() * config_.maxQueuePerServer;
            int qsize = (int)requestQueue_.size();
            int pct = capacity > 0 ? qsize * 100 / capacity : 0;

            std::ostringstream status;
            status << "Cycle " << cycle << "/" << config_.simulationCycles
                   << "  |  queue " << qsize << "/" << capacity << " (" << pct << "%)"
                   << "  |  servers=" << servers_.size()
                   << "  |  gen=" << stats_.generatedRequests
                   << " blocked=" << stats_.blockedRequests
                   << " done=" << stats_.completedRequests;
            logEvent(status.str());
        }
    }

    stats_.finalQueueSize = (int)requestQueue_.size();
    stats_.finalServerCount = (int)servers_.size();

    // write end-of-simulation summary to the log file
    if (logFile_.is_open()) {
        logFile_ << "\n[INFO] ==== Simulation Summary ====\n";
        logFile_ << "[INFO] Generated requests : " << stats_.generatedRequests  << '\n';
        logFile_ << "[INFO] Accepted requests  : " << stats_.acceptedRequests   << '\n';
        logFile_ << "[INFO] Blocked requests   : " << stats_.blockedRequests    << '\n';
        logFile_ << "[INFO] Completed requests : " << stats_.completedRequests  << '\n';
        logFile_ << "[INFO] Peak queue size    : " << stats_.peakQueueSize      << '\n';
        logFile_ << "[INFO] Final queue size   : " << stats_.finalQueueSize     << '\n';
        logFile_ << "[INFO] Servers added      : " << stats_.addedServers       << '\n';
        logFile_ << "[INFO] Servers removed    : " << stats_.removedServers     << '\n';
        logFile_ << "[INFO] Final server count : " << stats_.finalServerCount   << '\n';
        logFile_ << "[INFO] Log file           : " << config_.logFilePath       << '\n';
    }

    return stats_;
}

void LoadBalancer::initializeServers() {
    if (config_.initialServers < 1) {
        config_.initialServers = 1;
    }

    for (int index = 0; index < config_.initialServers; ++index) {
        addServer();
    }
}

void LoadBalancer::fillInitialQueue() {
    int targetQueueSize = config_.initialServers * config_.initialQueueMultiplier;
    while ((int)requestQueue_.size() < targetQueueSize)
        addRequest(generateRequest());

    stats_.peakQueueSize = (int)requestQueue_.size();
}

void LoadBalancer::maybeAddNewRequests() {
    std::uniform_int_distribution<int> chanceDist(1, 100);
    std::uniform_int_distribution<int> countDist(1, std::max(1, config_.maxNewRequestsPerCycle));

    if (chanceDist(generator_) <= config_.arrivalProbabilityPercent) {
        int count = countDist(generator_);
        for (int i = 0; i < count; ++i) {
            addRequest(generateRequest());
        }
    }
}

Request LoadBalancer::generateRequest() {
    return Request::randomRequest(nextRequestId_++, generator_, config_.minRequestTime, config_.maxRequestTime);
}