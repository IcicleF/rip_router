#if !defined(RIP_PACKET_HPP)
#define RIP_PACKET_HPP

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

#include "rip_entry.hpp"
#include "commons.hpp"

struct RIP_Packet {
    union {
        uint8_t buf[BUFLEN];
        struct {
            uint8_t cmd;
            uint8_t ver;
            uint16_t must_be_zero;
            struct RIPEntry entry[30];
        };
    };

    RIP_Packet() {
        memset(buf, 0, sizeof(buf));
    }
};

#endif // RIP_PACKET_HPP
