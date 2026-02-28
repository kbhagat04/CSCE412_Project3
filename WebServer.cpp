// WebServer.cpp

#include "WebServer.h"

// set up a new server with the given ID
WebServer::WebServer(const std::string& id) {
    serverId = id;
    isBusy = false;
    remainingTime = 0;
    currentRequest = nullptr;
    completedRequests = 0;
}

// clean up any leftover request
WebServer::~WebServer() {
    delete currentRequest;
}

// take a request if the server is free, copy it and start processing
bool WebServer::processRequest(Request* request) {
    if (request == nullptr || isBusy) {
        return false;
    }

    currentRequest = new Request(*request);
    remainingTime = currentRequest->timeRequired;
    isBusy = true;
    return true;
}

// count down the timer, return true if the request just finished
bool WebServer::processTick() {
    if (!isBusy) {
        return false;
    }

    remainingTime--;
    if (remainingTime <= 0) {
        delete currentRequest;
        currentRequest = nullptr;
        isBusy = false;
        completedRequests++;
        return true;
    }

    return false;
}

// returns true if server has no active request
bool WebServer::isAvailable() const {
    return !isBusy;
}

// getter for server ID
std::string WebServer::id() const {
    return serverId;
}

// getter for how many requests this server has finished
int WebServer::completedCount() const {
    return completedRequests;
}