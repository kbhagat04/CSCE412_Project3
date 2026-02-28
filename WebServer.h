/**
 * @file WebServer.h
 * @brief Defines the WebServer class used in the load balancer simulation.
 *
 * Each WebServer instance represents a single backend server that can handle
 * exactly one request at a time. The server counts down a processing timer
 * each clock cycle and becomes free once the timer reaches zero.
 *
 * @author Karan Bhagat
 * @date 2026
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include "Request.h"

/**
 * @class WebServer
 * @brief Represents one backend web server in the load balancer simulation.
 *
 * A WebServer can hold at most one active request at a time. Each call to
 * processTick() advances the internal countdown timer by one cycle. When
 * the timer hits zero the request is considered complete and the server
 * returns to an idle state.
 */
class WebServer {
public:
    /**
     * @brief Constructs a WebServer with the given identifier.
     * @param serverId Unique string label for this server (e.g. "S1").
     */
    WebServer(const std::string& serverId);

    /**
     * @brief Destructor. Releases any request currently held by this server.
     */
    ~WebServer();

    /**
     * @brief Assigns a request to this server if it is currently idle.
     * @param request Pointer to the Request to process.
     * @return @c true if the request was accepted; @c false if the server
     *         is already busy.
     */
    bool processRequest(Request* request);

    /**
     * @brief Advances the server by one simulation clock cycle.
     *
     * Decrements the remaining processing time of the current request by one.
     * When the timer reaches zero the request is finished, the completed
     * counter is incremented, and the server becomes idle again.
     *
     * @return @c true if a request completed during this tick; @c false
     *         otherwise.
     */
    bool processTick();

    /**
     * @brief Checks whether this server is currently idle.
     * @return @c true when no request is being processed.
     */
    bool isAvailable() const;

    /**
     * @brief Returns the unique identifier string for this server.
     * @return Server ID string set at construction time.
     */
    std::string id() const;

    /**
     * @brief Returns the total number of requests this server has finished.
     * @return Count of completed requests since the server was created.
     */
    int completedCount() const;

private:
    std::string serverId;      ///< Unique identifier for this server instance.
    bool isBusy;               ///< @c true while a request is being processed.
    int remainingTime;         ///< Clock cycles left until the current request finishes.
    Request* currentRequest;   ///< Pointer to the request currently being handled.
    int completedRequests;     ///< Running total of requests finished by this server.
};

#endif