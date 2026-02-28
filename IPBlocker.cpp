// IPBlocker.cpp

#include "IPBlocker.h"
#include <cstdlib>
#include <sstream>

// converts a dotted IP string like "10.0.0.1" into a 32-bit int
bool IPBlocker::parseIp(const std::string& ip, uint32_t& value) {
    std::stringstream ss(ip);
    std::string token;
    int octets[4] = {0, 0, 0, 0};
    int i = 0;

    while (std::getline(ss, token, '.')) {
        if (i >= 4 || token.empty()) {
            return false;
        }

        // make sure its all digits
        for (int j = 0; j < (int)token.size(); j++) {
            if (token[j] < '0' || token[j] > '9') {
                return false;
            }
        }

        int octet = atoi(token.c_str());
        if (octet < 0 || octet > 255) {
            return false;
        }
        octets[i++] = octet;
    }

    if (i != 4) {
        return false;
    }

    value = ((uint32_t)octets[0] << 24) | ((uint32_t)octets[1] << 16) | ((uint32_t)octets[2] << 8) | (uint32_t)octets[3];
    return true;
}

// adds a blocked range given explicit start and end IPs
bool IPBlocker::addBlockedRange(const std::string& startIp, const std::string& endIp) {
    uint32_t startVal = 0, endVal = 0;

    if (!parseIp(startIp, startVal) || !parseIp(endIp, endVal)) {
        return false;
    }

    if (startVal > endVal) {
        uint32_t temp = startVal;
        startVal = endVal;
        endVal = temp;
    }

    IpRange r;
    r.start = startVal;
    r.end = endVal;
    ranges.push_back(r);
    return true;
}

// parses a range string (CIDR or dash format) and adds it
bool IPBlocker::addBlockedRange(const std::string& spec) {
    // range format: "1.2.3.4-5.6.7.8"
    int dashPos = (int)spec.find('-');
    if (dashPos != -1) {
        return addBlockedRange(spec.substr(0, dashPos), spec.substr(dashPos + 1));
    }

    // CIDR format: "10.0.0.0/8"
    int slashPos = (int)spec.find('/');
    if (slashPos != -1) {
        std::string ipPart = spec.substr(0, slashPos);
        int prefixLen = atoi(spec.substr(slashPos + 1).c_str());

        if (prefixLen < 0 || prefixLen > 32) {
            return false;
        }

        uint32_t baseIp = 0;
        if (!parseIp(ipPart, baseIp)) {
            return false;
        }

        uint32_t mask;
        if (prefixLen == 0) {
            mask = 0;
        } else {
            mask = 0xFFFFFFFFu << (32 - prefixLen);
        }
        uint32_t startVal = baseIp & mask;
        uint32_t endVal = startVal | ~mask;

        IpRange r;
        r.start = startVal;
        r.end = endVal;
        ranges.push_back(r);
        return true;
    }

    return addBlockedRange(spec, spec);
}

// returns true if the given IP falls inside any blocked range
bool IPBlocker::isBlocked(const std::string& ip) const {
    uint32_t ipVal = 0;
    if (!parseIp(ip, ipVal)) {
        return true;
    }

    for (int i = 0; i < (int)ranges.size(); i++) {
        if (ipVal >= ranges[i].start && ipVal <= ranges[i].end) {
            return true;
        }
    }
    return false;
}