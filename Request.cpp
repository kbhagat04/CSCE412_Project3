// Request.cpp

#include "Request.h"
#include <cstdlib>

// default constructor - zeroes everything out
Request::Request() {
    id = 0;
    ipIn = "0.0.0.0";
    ipOut = "0.0.0.0";
    timeRequired = 0;
    jobType = 'P';
}

// generates a random IP address like "192.168.1.55"
std::string Request::randomIp() {
    std::string ip = std::to_string(rand() % 256) + "." + std::to_string(rand() % 256) + "." + std::to_string(rand() % 256) + "." + std::to_string(rand() % 256);
    return ip;
}

// builds a random request with the given ID and time range
Request Request::randomRequest(int nextId, int minTime, int maxTime) {
    Request request;
    request.id = nextId;
    request.ipIn = randomIp();
    request.ipOut = randomIp();
    request.timeRequired = minTime + rand() % (maxTime - minTime + 1);
    if (rand() % 2 == 0) {
        request.jobType = 'P';
    } else {
        request.jobType = 'S';
    }
    return request;
}