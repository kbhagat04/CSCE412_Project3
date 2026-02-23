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
class IPBlocker {
public:
    /**
     * @brief Block a range using two IP strings as start and end.
     * @param startIp First IP in the range.
     * @param endIp Last IP in the range.
     * @return false if either IP string is bad.
     */
    bool addBlockedRange(const std::string& startIp, const std::string& endIp);

    /**
     * @brief Block a range from a single string.
     *        Supports "1.2.3.4-5.6.7.8" or CIDR like "10.0.0.0/8".
     * @param spec The range string.
     * @return false if format is invalid.
     */
    bool addBlockedRange(const std::string& spec);

    /**
     * @brief Check if an IP should be blocked.
     * @param ip The source IP to check.
     * @return true if it matches a blocked range.
     */
    bool isBlocked(const std::string& ip) const;

private:
    std::vector<std::string> blockedRanges_;                           // human-readable list
    std::vector<std::pair<uint32_t, uint32_t>> parsedRanges_;         // numeric start/end pairs

    static bool parseIp(const std::string& ip, uint32_t& value);
};

#endif