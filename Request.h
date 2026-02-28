/**
 * @file Request.h
 * @brief Defines the Request struct that represents a single web request
 *        flowing through the load balancer.
 *
 * @author Karan Bhagat
 * @date 2026
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <string>

/**
 * @struct Request
 * @brief Holds all data associated with a single web request in the simulation.
 *
 * Requests are created by the LoadBalancer, queued, and eventually assigned
 * to an idle WebServer for processing. Each request carries source and
 * destination IP addresses, an estimated processing time (in clock cycles),
 * and a job-type flag that categorizes the workload.
 */
struct Request {
    int id;              ///< Unique sequential identifier assigned at generation time.
    std::string ipIn;    ///< Source (client) IP address in dotted-decimal notation.
    std::string ipOut;   ///< Destination (server) IP address in dotted-decimal notation.
    int timeRequired;    ///< Number of clock cycles needed to process this request.
    char jobType;        ///< Workload category: @c 'P' for processing, @c 'S' for streaming.

    /**
     * @brief Default constructor. Initializes all fields to safe zero/empty values.
     */
    Request();

    /**
     * @brief Generates a random IPv4 address string.
     *
     * Each octet is independently chosen from [0, 255], producing strings
     * of the form @c "A.B.C.D".
     *
     * @return Random IPv4 address as a std::string.
     */
    static std::string randomIp();

    /**
     * @brief Factory method that produces a fully populated Request with random values.
     *
     * Randomly generates source and destination IP addresses, selects a
     * processing time uniformly from [minTime, maxTime], and randomly
     * assigns the job type as either @c 'P' or @c 'S'.
     * @param nextId      Sequential ID to assign to the new request.
     * @param minTime     Minimum processing time (inclusive, in clock cycles).
     * @param maxTime     Maximum processing time (inclusive, in clock cycles).
     * @return            A fully initialized Request object.
     */
    static Request randomRequest(int nextId, int minTime, int maxTime);
};

#endif