/**
 * @file Request.h
 * @brief Holds the Request struct used throughout the simulation.
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <random>
#include <string>

/**
 * @struct Request
 * @brief Holds all the data for one web request.
 */
struct Request {
    int id;              // unique request ID
    std::string ipIn;    // source IP
    std::string ipOut;   // destination IP
    int timeRequired;    // how many cycles to process
    char jobType;        // 'P' for processing, 'S' for streaming

    /**
     * @brief Default constructor.
     */
    Request();

    /**
     * @brief Builds a request with random IPs, time, and job type.
     * @param nextId ID to assign to this request.
     * @param generator RNG to use.
     * @param minTime Min processing time.
     * @param maxTime Max processing time.
     * @return A filled-in Request object.
     */
    static Request randomRequest(int nextId, std::mt19937& generator, int minTime, int maxTime);

    /**
     * @brief Generates a random IPv4 address.
     * @param generator RNG to use.
     * @return IP address string like "192.168.1.1".
     */
    static std::string randomIp(std::mt19937& generator);
};

#endif