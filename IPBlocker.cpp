/**
 * @file IPBlocker.cpp
 * @brief Implementation of IP range blocking / firewall logic.
 */

#include "IPBlocker.h"

#include <algorithm>
#include <sstream>

bool IPBlocker::parseIp(const std::string& ip, std::uint32_t& value) {
    std::stringstream stream(ip);
    std::string token;

    int octets[4] = {0, 0, 0, 0};
    int index = 0;
    while (std::getline(stream, token, '.')) {
        if (index >= 4 || token.empty()) {
            return false;
        }

        for (char character : token) {
            if (character < '0' || character > '9') {
                return false;
            }
        }

        int octet = std::stoi(token);
        if (octet < 0 || octet > 255) {
            return false;
        }

        octets[index++] = octet;
    }

    if (index != 4) {
        return false;
    }

    value = (static_cast<std::uint32_t>(octets[0]) << 24) |
            (static_cast<std::uint32_t>(octets[1]) << 16) |
            (static_cast<std::uint32_t>(octets[2]) << 8) |
            static_cast<std::uint32_t>(octets[3]);
    return true;
}

bool IPBlocker::addBlockedRange(const std::string& startIp, const std::string& endIp) {
    std::uint32_t startVal = 0;
    std::uint32_t endVal = 0;

    if (!parseIp(startIp, startVal) || !parseIp(endIp, endVal)) {
        return false;
    }

    if (startVal > endVal) {
        std::swap(startVal, endVal);
    }

    blockedRanges_.push_back(startIp + "-" + endIp);
    parsedRanges_.push_back({startVal, endVal});
    return true;
}

bool IPBlocker::addBlockedRange(const std::string& spec) {
    auto dashPos = spec.find('-');
    if (dashPos != std::string::npos) {
        std::string left = spec.substr(0, dashPos);
        std::string right = spec.substr(dashPos + 1);
        return addBlockedRange(left, right);
    }

    auto slashPos = spec.find('/');
    if (slashPos != std::string::npos) {
        std::string ipPart = spec.substr(0, slashPos);
        std::string prefixPart = spec.substr(slashPos + 1);

        std::uint32_t baseIp = 0;
        if (!parseIp(ipPart, baseIp)) {
            return false;
        }

        int prefixLen = std::stoi(prefixPart);
        if (prefixLen < 0 || prefixLen > 32) {
            return false;
        }

        std::uint32_t mask = prefixLen == 0 ? 0 : 0xFFFFFFFFu << (32 - prefixLen);
        std::uint32_t startVal = baseIp & mask;
        std::uint32_t endVal = startVal | ~mask;

        blockedRanges_.push_back(spec);
        parsedRanges_.push_back({startVal, endVal});
        return true;
    }

    return addBlockedRange(spec, spec);
}

bool IPBlocker::isBlocked(const std::string& ip) const {
    std::uint32_t ipValue = 0;
    if (!parseIp(ip, ipValue)) {
        return true;
    }

    for (const auto& range : parsedRanges_) {
        if (ipValue >= range.first && ipValue <= range.second) {
            return true;
        }
    }

    return false;
}