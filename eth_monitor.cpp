#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <unistd.h>

#include "eth_monitor.hpp"
#include "global.hpp"

using namespace std;

const int MAX_LEN = 1500;

// Checksum functions
union HdrStruct {
    ip ipFormat;
    uint16_t shortFormat[10];
};
inline uint16_t getChecksum(const HdrStruct& hdr) {
    uint32_t sum = 0;
    for (int i = 0; i < hdr.ipFormat.ip_hl * 2; ++i)
        sum += hdr.shortFormat[i];
    sum = (sum >> 16) + (sum & 0xFFFF);
    uint16_t chksum = ~sum;
    return chksum;
}
inline bool validateChecksum(const ip& iphdr) {
    HdrStruct hdr;
    hdr.ipFormat = iphdr;
    return getChecksum(hdr) == 0;
}
inline void updateChecksum(ip& iphdr) {
    HdrStruct hdr;
    hdr.ipFormat = iphdr;
    hdr.ipFormat.ip_sum = 0;
    uint16_t chksum = getChecksum(hdr);
    iphdr.ip_sum = chksum;
}

// Nexthop
struct NextHopInfo {
    int ifIndex;
    uint8_t macFrom[6];
    uint8_t macTo[6];
};
int arpGet(uint8_t* mac, const string& ifName, const in_addr& ipAddr) {
    arpreq arpRequest;
    memset(&arpRequest, 0, sizeof(arpRequest));

    sockaddr_in* sin = (sockaddr_in*)&(arpRequest.arp_pa);
    sin->sin_family = AF_INET;
    sin->sin_addr = ipAddr;
    strncpy(arpRequest.arp_dev, ifName.c_str(), IF_NAMESIZE - 1);

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    int ret = -1;
    if (ioctl(sfd, SIOCGARP, &arpRequest) >= 0)
        if (arpRequest.arp_flags & ATF_COM) {
            memcpy(mac, arpRequest.arp_ha.sa_data, ETH_ALEN);
            ret = 0;
        }
    close(sfd);
    return ret;
}
bool lookupRoute(in_addr dst, NextHopInfo* res) {
    for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next) {
        if (dst.s_addr == ifc->addr.s_addr) {
            //printf("[EthMon] pakcet to self, do not forward\n");
            return false;
        }
        if ((dst.s_addr & ifc->mask.s_addr) == (ifc->addr.s_addr & ifc->mask.s_addr)) {
            res->ifIndex = ifc->index;
            memcpy(res->macFrom, ifc->mac, ETH_ALEN);
            return arpGet(res->macTo, ifc->name, dst) == 0;
        }
    }
    for (RT* rt = gl->rt_head; rt; rt = rt->next)
        if ((dst.s_addr & rt->mask) == (rt->addr & rt->mask)) {
            Interface* ifc = NULL;
            for (ifc = gl->if_head; ifc; ifc = ifc->next)
                if ((rt->nextHop & ifc->mask.s_addr) == (ifc->addr.s_addr & ifc->mask.s_addr))
                    break;
            if (ifc) {
                res->ifIndex = ifc->index;
                memcpy(res->macFrom, ifc->mac, ETH_ALEN);
                return arpGet(res->macTo, ifc->name, (in_addr){rt->nextHop}) == 0;
            }
            break;
        }
    return false;
}

void eth_monitor() {
    char pbuf[MAX_LEN], data[MAX_LEN];
    int recvfd, recvlen;
    if ((recvfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        printf("%s\n", strerror(errno));
        return;
    }

    NextHopInfo nhi;
    printf("\033[36m[EthMon]\033[0m Started.\n");
    while (true) {
        recvlen = recv(recvfd, pbuf, sizeof(pbuf), 0);
        if (recvlen <= 0)
            continue;
        ethhdr* eh = (ethhdr*)pbuf;
        ip* recvhdr = (ip*)(pbuf + sizeof(ether_header));
        in_addr_t dstAddrType = recvhdr->ip_dst.s_addr & 0xFF;
        if (dstAddrType != 0x7F && dstAddrType != 0xE0) {
            printf("\033[36m[EthMon]\033[0m received packet to %s\n", inet_ntoa(recvhdr->ip_dst));
            if (!validateChecksum(*recvhdr))
                continue;
            if (recvhdr->ip_ttl == 0)
                continue;
            --(recvhdr->ip_ttl);
            updateChecksum(*recvhdr);
        
            if (!lookupRoute(recvhdr->ip_dst, &nhi))
                continue;
            printf("\033[36m[EthMon]\033[0m found next hop via if_index %d\n", nhi.ifIndex);
            memcpy(eh->h_dest, nhi.macTo, ETH_ALEN);
            memcpy(eh->h_source, nhi.macFrom, ETH_ALEN);
            eh->h_proto = htons(ETHERTYPE_IP);

            sockaddr_ll saddr;
            saddr.sll_family = AF_PACKET;
            saddr.sll_ifindex = nhi.ifIndex;
            saddr.sll_halen = ETH_ALEN;
            memcpy(saddr.sll_addr, nhi.macFrom, ETH_ALEN);
            if (sendto(recvfd, pbuf, recvlen, 0, (sockaddr*)(&saddr), sizeof(saddr)) < 0)
                perror("sendto");
        }
    }
}