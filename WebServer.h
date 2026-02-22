/**
 * @file WebServer.h
 * @brief WebServer class - each server handles one request at a time.
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <string>

#include "Request.h"

/**
 * @class WebServer
 * @brief Represents one web server in the simulation.
 *        Can only handle one request at a time.
 */
class WebServer {
public:
    /**
     * @brief Constructor, give it an ID string.
     * @param serverId Name/ID for this server.
     */
    explicit WebServer(const std::string& serverId);

    /**
     * @brief Give this server a request to work on.
     * @param request Pointer to the request.
     * @return true if it took it, false if already busy.
     */
    bool processRequest(Request* request);

    /**
     * @brief Called every clock cycle to count down the request timer.
     * @return true if a request just finished.
     */
    bool processTick();

    /**
     * @brief Check if this server is free.
     * @return true if no active request.
     */
    bool isAvailable() const;

    /**
     * @brief Returns the server's ID.
     */
    std::string id() const;

    /**
     * @brief How many requests this server has finished.
     */
    int completedCount() const;

private:
    std::string serverId_;
    bool isBusy_;
    int remainingTime_;
    std::unique_ptr<Request> currentRequest_;
    int completedCount_;
};

#endif