// Request.cpp

#include "Request.h"
#include <sstream>

Request::Request() : id(0), ipIn("0.0.0.0"), ipOut("0.0.0.0"), timeRequired(0), jobType('P') {}

Request Request::randomRequest(int nextId, std::mt19937& generator, int minTime, int maxTime) {
    std::uniform_int_distribution<int> timeDist(minTime, maxTime);
    std::uniform_int_distribution<int> typeDist(0, 1);

    Request request;
    request.id = nextId;
    request.ipIn = randomIp(generator);
    request.ipOut = randomIp(generator);
    request.timeRequired = timeDist(generator);
    request.jobType = typeDist(generator) == 0 ? 'P' : 'S';
    return request;
}

std::string Request::randomIp(std::mt19937& generator) {
    std::uniform_int_distribution<int> octetDist(0, 255);
    std::ostringstream stream;
    stream << octetDist(generator) << '.' << octetDist(generator) << '.' << octetDist(generator) << '.' << octetDist(generator);
    return stream.str();
}