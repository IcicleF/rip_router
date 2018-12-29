#if !defined(INTERFACES_HPP)
#define INTERFACES_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "commons.hpp"

struct Interface {
    std::string name;
    int index;
    in_addr addr;
    in_addr mask;
    uint8_t mac[8];
    int sockfd;

    Interface* next;

    Interface() : next(NULL) { }
    bool operator==(const Interface& b) const {
        return (name == b.name) && (index == b.index) && (addr.s_addr == b.addr.s_addr) && (mask.s_addr == b.mask.s_addr)
            && (memcmp(mac, b.mac, 6) == 0);
    }
};

#endif // INTERFACES_HPP
