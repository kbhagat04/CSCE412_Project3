// IPBlocker.cpp
// IP range blocking logic - used to filter out requests from bad IPs

#include "IPBlocker.h"
#include <sstream>
#include <algorithm>

bool IPBlocker::parseIp(const std::string& ip, uint32_t& value) {
    std::stringstream ss(ip);
    std::string token;
    int octets[4] = {0, 0, 0, 0};
    int i = 0;

    while (std::getline(ss, token, '.')) {
        if (i >= 4 || token.empty()) return false;

        // make sure its all digits
        for (char c : token) {
            if (c < '0' || c > '9') return false;
        }

        int octet = std::stoi(token);
        if (octet < 0 || octet > 255) return false;
        octets[i++] = octet;
    }

    if (i != 4) return false;

    value = ((uint32_t)octets[0] << 24) | ((uint32_t)octets[1] << 16)
          | ((uint32_t)octets[2] << 8)  |  (uint32_t)octets[3];
    return true;
}

bool IPBlocker::addBlockedRange(const std::string& startIp, const std::string& endIp) {
    uint32_t startVal = 0, endVal = 0;

    if (!parseIp(startIp, startVal) || !parseIp(endIp, endVal))
        return false;

    if (startVal > endVal) std::swap(startVal, endVal);

    blockedRanges_.push_back(startIp + "-" + endIp);
    parsedRanges_.push_back({startVal, endVal});
    return true;
}

bool IPBlocker::addBlockedRange(const std::string& spec) {
    // check for range format: "1.2.3.4-5.6.7.8"
    auto dashPos = spec.find('-');
    if (dashPos != std::string::npos) {
        return addBlockedRange(spec.substr(0, dashPos), spec.substr(dashPos + 1));
    }

    // check for CIDR format: "10.0.0.0/8"
    auto slashPos = spec.find('/');
    if (slashPos != std::string::npos) {
        std::string ipPart = spec.substr(0, slashPos);
        int prefixLen = std::stoi(spec.substr(slashPos + 1));

        if (prefixLen < 0 || prefixLen > 32) return false;

        uint32_t baseIp = 0;
        if (!parseIp(ipPart, baseIp)) return false;

        uint32_t mask = prefixLen == 0 ? 0 : 0xFFFFFFFFu << (32 - prefixLen);
        uint32_t startVal = baseIp & mask;
        uint32_t endVal = startVal | ~mask;

        blockedRanges_.push_back(spec);
        parsedRanges_.push_back({startVal, endVal});
        return true;
    }

    // treat as single IP
    return addBlockedRange(spec, spec);
}

bool IPBlocker::isBlocked(const std::string& ip) const {
    uint32_t ipVal = 0;
    if (!parseIp(ip, ipVal)) return true;  // treat malformed IPs as blocked

    for (auto& range : parsedRanges_) {
        if (ipVal >= range.first && ipVal <= range.second)
            return true;
    }
    return false;
}