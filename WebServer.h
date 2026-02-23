// WebServer.h
// represents one web server - can only hold one request at a time

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include "Request.h"

/**
 * @class WebServer
 * @brief Represents one web server in the simulation.
 *        Can only handle one request at a time.
 */
class WebServer {
public:
    WebServer(const std::string& serverId);
    ~WebServer();

    /**
     * @brief Give this server a request to work on.
     * @param request Pointer to the request.
     * @return true if it took it, false if already busy.
     */
    bool processRequest(Request* request);

    /**
     * @brief Called every clock cycle to count down the request timer.
     * @return true if the request just finished.
     */
    bool processTick();

    // returns true if server has no active request
    bool isAvailable() const;

    std::string id() const;
    int completedCount() const;

private:
    std::string serverId_;
    bool isBusy_;
    int remainingTime_;
    Request* currentRequest_;   // raw pointer, we manage the memory
    int completedCount_;
};

#endif