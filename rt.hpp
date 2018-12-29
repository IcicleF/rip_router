#if !defined(RT_HPP)
#define RT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstdio>

#include "commons.hpp"

struct RT {
    in_addr_t addr;
    in_addr_t mask;
    in_addr_t nextHop;
    uint32_t metric;

    TimePoint expire;
    RT* next;

    RT() : next(NULL) { }
};

#endif // RT_HPP
