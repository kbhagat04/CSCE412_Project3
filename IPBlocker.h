/**
 * @file IPBlocker.h
 * @brief Defines the IPBlocker class and IpRange helper struct used to
 *        implement a simple IP firewall for the load balancer simulation.
 *
 * @author Karan Bhagat
 * @date 2026
 */

#ifndef IPBLOCKER_H
#define IPBLOCKER_H

#include <string>
#include <vector>
#include <cstdint>

/**
 * @struct IpRange
 * @brief Represents a contiguous range of IPv4 addresses stored as
 *        packed 32-bit unsigned integers for fast comparison.
 */
struct IpRange {
    uint32_t start; ///< Lowest address in the range (network byte order).
    uint32_t end;   ///< Highest address in the range (inclusive).
};

/**
 * @class IPBlocker
 * @brief Simple IPv4 firewall that rejects traffic from blocked address ranges.
 *
 * Ranges are added either as explicit start/end pairs or as CIDR notation
 * strings (e.g. @c "10.0.0.0/8") or dash-separated ranges
 * (e.g. @c "192.168.1.1-192.168.1.20"). All addresses are stored internally
 * as packed @c uint32_t values so membership tests are O(n) comparisons
 * with no allocation.
 */
class IPBlocker {
public:
    /**
     * @brief Adds a blocked range defined by two explicit IP address strings.
     * @param startIp First (lowest) address of the range in dotted-decimal notation.
     * @param endIp   Last (highest) address of the range in dotted-decimal notation.
     * @return @c true if both addresses were parsed successfully and the
     *         range was added; @c false on parse failure.
     */
    bool addBlockedRange(const std::string& startIp, const std::string& endIp);

    /**
     * @brief Adds a blocked range from a single specification string.
     *
     * Accepts two formats:
     * - CIDR:  @c "10.0.0.0/8"
     * - Range: @c "192.168.1.1-192.168.1.20"
     *
     * @param spec Range specification string.
     * @return @c true if the spec was parsed and added; @c false otherwise.
     */
    bool addBlockedRange(const std::string& spec);

    /**
     * @brief Tests whether an IPv4 address falls within any blocked range.
     * @param ip Address to test in dotted-decimal notation (e.g. @c "10.5.6.7").
     * @return @c true if the address is blocked; @c false if it is allowed.
     */
    bool isBlocked(const std::string& ip) const;

private:
    std::vector<IpRange> ranges; ///< List of all registered blocked IP ranges.

    /**
     * @brief Converts a dotted-decimal IPv4 string into a packed 32-bit integer.
     * @param ip    Input address string (e.g. @c "192.168.1.1").
     * @param value Output parameter filled with the packed integer on success.
     * @return @c true if parsing succeeded; @c false on malformed input.
     */
    static bool parseIp(const std::string& ip, uint32_t& value);
};

#endif