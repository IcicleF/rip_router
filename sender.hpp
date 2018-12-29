#if !defined(SENDER_HPP)
#define SENDER_HPP

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/ip.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rip_packet.hpp"
#include "interface.hpp"

void sendRIP(uint8_t type, const Interface* ifc = NULL);

#endif // SENDER_HPP
