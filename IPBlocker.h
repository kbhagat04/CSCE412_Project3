// IPBlocker.h
// checks if an incoming IP is in a blocked range (acts as a basic firewall)

#ifndef IPBLOCKER_H
#define IPBLOCKER_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * @class IPBlocker
 * @brief Keeps a list of blocked IP ranges and checks incoming IPs against them.
 */
struct IpRange {
    uint32_t start;
    uint32_t end;
};

class IPBlocker {
public:
    bool addBlockedRange(const std::string& startIp, const std::string& endIp);
    bool addBlockedRange(const std::string& spec);
    bool isBlocked(const std::string& ip) const;

private:
    std::vector<IpRange> ranges_;

    static bool parseIp(const std::string& ip, uint32_t& value);
};

#endif