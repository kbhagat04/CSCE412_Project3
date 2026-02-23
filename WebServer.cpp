// WebServer.cpp
// handles assigning requests and counting down each clock tick

#include "WebServer.h"

WebServer::WebServer(const std::string& serverId)
    : serverId_(serverId), isBusy_(false), remainingTime_(0), currentRequest_(nullptr), completedCount_(0) {}

WebServer::~WebServer() {
    delete currentRequest_;
}

bool WebServer::processRequest(Request* request) {
    if (request == nullptr || isBusy_) {
        return false;
    }

    // copy the request so we own it
    currentRequest_ = new Request(*request);
    remainingTime_ = currentRequest_->timeRequired;
    isBusy_ = true;
    return true;
}

bool WebServer::processTick() {
    if (!isBusy_) return false;

    remainingTime_--;
    if (remainingTime_ <= 0) {
        delete currentRequest_;
        currentRequest_ = nullptr;
        isBusy_ = false;
        completedCount_++;
        return true;
    }

    return false;
}

bool WebServer::isAvailable() const {
    return !isBusy_;
}

std::string WebServer::id() const {
    return serverId_;
}

int WebServer::completedCount() const {
    return completedCount_;
}