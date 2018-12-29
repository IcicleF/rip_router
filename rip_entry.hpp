#if !defined(RIP_ENTRY_HPP)
#define RIP_ENTRY_HPP

#include <netinet/in.h>

struct RIPEntry {
    uint16_t afi;
    uint16_t rtag;
    in_addr_t addr;
    in_addr_t mask;
    in_addr_t nextHop;
    uint32_t metric;
    
    void reverseEndian() {
        afi = __builtin_bswap16(afi);
        rtag = __builtin_bswap16(rtag);
        metric = __builtin_bswap32(metric);
    }
};

#endif // RIP_ENTRY_HPP
