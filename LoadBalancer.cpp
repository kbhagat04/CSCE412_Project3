// LoadBalancer.cpp

#include "LoadBalancer.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

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
    for (int i = 0; i < (int)servers_.size(); i++) {
        delete servers_[i];
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
        std::ostringstream msg;
        msg << "Request #" << request.id << " BLOCKED | src=" << request.ipIn << " dst=" << request.ipOut;
        writeLog("BLOCK", YELLOW, msg.str());
        return;
    }

    requestQueue_.push(request);
    stats_.acceptedRequests++;
    if (logFile_.is_open()) {
        logFile_ << "[QUEUED] Request #" << request.id << " | " << request.ipIn << " -> " << request.ipOut << " | type=" << request.jobType << " time=" << request.timeRequired << '\n';
    }
}

void LoadBalancer::processTick() {
    for (int i = 0; i < (int)servers_.size(); i++) {
        if (requestQueue_.empty()) {
            break;
        }

        if (servers_[i]->isAvailable()) {
            Request next = requestQueue_.front();
            requestQueue_.pop();
            // file-only dispatch log
            if (logFile_.is_open()) {
                logFile_ << "[ASSIGNED] Request #" << next.id << " -> server " << servers_[i]->id() << " | " << next.ipIn << " -> " << next.ipOut << " | time=" << next.timeRequired << '\n';
            }
            servers_[i]->processRequest(&next);
        }
    }

    for (int i = 0; i < (int)servers_.size(); i++) {
        if (servers_[i]->processTick()) {
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
    const int MIN_QUEUE_PER_SERVER = 50;
    const int MAX_QUEUE_PER_SERVER = 80;
    int lowerThreshold = MIN_QUEUE_PER_SERVER * serverCount;
    int upperThreshold = MAX_QUEUE_PER_SERVER * serverCount;

    if (queueSize > upperThreshold) {
        addServer();
        stats_.addedServers++;
        cooldownTimer_ = config_.scalingCooldownCycles;
        std::ostringstream msg;
        msg << "Cycle " << currentTime_ << ": queue=" << queueSize << " exceeded max threshold=" << upperThreshold << ", added 1 server (now " << servers_.size() << ")";
        writeLog("SCALE UP", GREEN, msg.str());
    } else if (queueSize < lowerThreshold && serverCount > 1) {
        if (removeServer()) {
            stats_.removedServers++;
            cooldownTimer_ = config_.scalingCooldownCycles;
            std::ostringstream msg;
            msg << "Cycle " << currentTime_ << ": queue=" << queueSize << " below min threshold=" << lowerThreshold << ", removed 1 server (now " << servers_.size() << ")";
            writeLog("SCALE DOWN", RED, msg.str());
        }
    }
}

void LoadBalancer::addServer() {
    std::string id = std::to_string(servers_.size() + 1);
    servers_.push_back(new WebServer(id));
}

bool LoadBalancer::removeServer() {
    // find an available server to remove (search from back to front)
    for (int i = (int)servers_.size() - 1; i >= 0; i--) {
        if (servers_[i]->isAvailable()) {
            WebServer* target = servers_[i];
            servers_.erase(servers_.begin() + i);
            delete target;
            return true;
        }
    }
    return false;
}

SimulationStats LoadBalancer::run() {
    initializeServers();

    std::ostringstream banner;
    banner << "Starting simulation for " << config_.simulationCycles << " cycles with " << servers_.size() << " server(s)";
    logEvent(banner.str());

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

    std::ostringstream qinfo;
    qinfo << "Initial queue: " << requestQueue_.size() << " requests" << " | generated=" << stats_.generatedRequests << " | blocked=" << stats_.blockedRequests << " | accepted=" << stats_.acceptedRequests;
    logInfo(qinfo.str());

    const int MAX_QUEUE_PER_SERVER = 80;
    const int MIN_QUEUE_PER_SERVER = 50;
    int cap = (int)servers_.size() * MAX_QUEUE_PER_SERVER;
    int fillPct = cap > 0 ? (int)(requestQueue_.size() * 100 / cap) : 0;
    std::ostringstream capinfo;
    capinfo << "Queue capacity: " << cap << " (" << MAX_QUEUE_PER_SERVER << " per server)" << " | fill=" << fillPct << "%" << "  [scale-up >" << MAX_QUEUE_PER_SERVER << "/srv, scale-down <" << MIN_QUEUE_PER_SERVER << "/srv]";
    logInfo(capinfo.str());

    for (int cycle = 1; cycle <= config_.simulationCycles; ++cycle) {
        currentTime_ = cycle;
        maybeAddNewRequests();
        processTick();

        if ((int)requestQueue_.size() > stats_.peakQueueSize)
            stats_.peakQueueSize = (int)requestQueue_.size();

        balanceLoad();

        if (config_.statusPrintInterval > 0 && cycle % config_.statusPrintInterval == 0) {
            const int MAX_QUEUE_PER_SERVER = 80;
            int capacity = (int)servers_.size() * MAX_QUEUE_PER_SERVER;
            int qsize = (int)requestQueue_.size();
            int pct = capacity > 0 ? qsize * 100 / capacity : 0;

            std::ostringstream status;
            status << "Cycle " << cycle << "/" << config_.simulationCycles << "  |  queue " << qsize << "/" << capacity << " (" << pct << "%)" << "  |  servers=" << servers_.size() << "  |  gen=" << stats_.generatedRequests << " blocked=" << stats_.blockedRequests << " done=" << stats_.completedRequests;
            logEvent(status.str());
        }
    }

    stats_.finalQueueSize = (int)requestQueue_.size();
    stats_.finalServerCount = (int)servers_.size();

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
    if (rand() % 2 == 0) {
        addRequest(generateRequest());
    }
}

Request LoadBalancer::generateRequest() {
    return Request::randomRequest(nextRequestId_++, generator_, config_.minRequestTime, config_.maxRequestTime);
}