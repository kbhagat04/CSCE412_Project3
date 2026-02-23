// LoadBalancer.cpp
// implementation of LoadBalancer - queue management, server scaling, logging

#include "LoadBalancer.h"
#include <algorithm>
#include <iostream>
#include <sstream>

// ANSI color codes for terminal output
#define RESET  "\033[0m"
#define CYAN   "\033[36m"
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
        return;
    }

    requestQueue_.push(request);
    stats_.acceptedRequests++;
}

void LoadBalancer::processTick() {
    for (WebServer* server : servers_) {
        if (requestQueue_.empty()) {
            break;
        }

        if (server->isAvailable()) {
            Request next = requestQueue_.front();
            requestQueue_.pop();
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
        logWarning("Cycle " + std::to_string(currentTime_) +
                   ": queue above threshold, added 1 server (now " +
                   std::to_string(servers_.size()) + ")");
    } else if (queueSize < lowerThreshold && serverCount > 1) {
        if (removeServer()) {
            stats_.removedServers++;
            cooldownTimer_ = config_.scalingCooldownCycles;
            logWarning("Cycle " + std::to_string(currentTime_) +
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
    fillInitialQueue();

    logEvent("Starting simulation for " + std::to_string(config_.simulationCycles) +
             " cycles with " + std::to_string(servers_.size()) + " server(s)");

    for (int cycle = 1; cycle <= config_.simulationCycles; ++cycle) {
        currentTime_ = cycle;
        maybeAddNewRequests();
        processTick();

        if ((int)requestQueue_.size() > stats_.peakQueueSize)
            stats_.peakQueueSize = (int)requestQueue_.size();

        balanceLoad();

        if (config_.statusPrintInterval > 0 && cycle % config_.statusPrintInterval == 0) {
            std::ostringstream status;
            status << "Cycle " << cycle << " | queue=" << requestQueue_.size()
                   << " | servers=" << servers_.size() << " | completed=" << stats_.completedRequests;
            logEvent(status.str());
        }
    }

    stats_.finalQueueSize = (int)requestQueue_.size();
    stats_.finalServerCount = (int)servers_.size();

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