#include "sender.hpp"
#include "global.hpp"

using namespace std;

inline int sendRIPTo(void* buf, int len, int sockfd) {
    sockaddr_in saddr;
    memset(&saddr, 0, sizeof(sockaddr_in));
    saddr.sin_addr.s_addr = inet_addr("224.0.0.9");
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(520);
    return sendto(sockfd, buf, len, 0, (sockaddr*)(&saddr), sizeof(saddr));
}

inline int computeRIPTo(RIP_Packet* rip, in_addr_t addr, in_addr_t mask) {
    int cnt = 0;
    for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next) {
        if ((ifc->addr.s_addr & ifc->mask.s_addr) == (addr & mask))
            continue;
        rip->entry[cnt].afi = 2;
        rip->entry[cnt].rtag = 0;
        rip->entry[cnt].addr = ifc->addr.s_addr & ifc->mask.s_addr;
        rip->entry[cnt].mask = ifc->mask.s_addr;
        rip->entry[cnt].metric = 1;
        rip->entry[cnt].nextHop = 0x0;
        ++cnt;
    }
    for (RT* rt = gl->rt_head; rt; rt = rt->next) {
        if ((rt->nextHop & rt->mask) == (addr & mask))
            continue;
        rip->entry[cnt].afi = 2;
        rip->entry[cnt].rtag = 0;
        rip->entry[cnt].addr = rt->addr & rt->mask;
        rip->entry[cnt].mask = rt->mask;
        rip->entry[cnt].metric = rt->metric;
        rip->entry[cnt].nextHop = rt->nextHop;
        ++cnt;
    }
    for (int i = 0; i < cnt; ++i)
        rip->entry[i].reverseEndian();
    return cnt * 20 + 4;
}

void sendRIP(uint8_t type, const Interface* ifc) {
    RIP_Packet rip;
    rip.ver = 2;
    if (type == RIP_REQUEST) {
        rip.cmd = RIP_REQUEST;
        rip.entry[0].metric = 16;
        rip.entry[0].reverseEndian();
        for (Interface* i = gl->if_head; i; i = i->next)
            sendRIPTo(rip.buf, 24, i->sockfd);
    }
    else {
        rip.cmd = RIP_RESPONSE;
        if (ifc) {
            int len = computeRIPTo(&rip, ifc->addr.s_addr, ifc->mask.s_addr);
            sendRIPTo(rip.buf, len, ifc->sockfd);
        }
        else
            for (Interface* i = gl->if_head; i; i = i->next) {
                int len = computeRIPTo(&rip, i->addr.s_addr, i->mask.s_addr);
                sendRIPTo(rip.buf, len, i->sockfd);
            }
    }
}