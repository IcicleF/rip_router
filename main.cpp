#include <cstdlib>
#include <cstdio>

#include "net_monitor.hpp"
#include "rip_packet.hpp"
#include "sender.hpp"
#include "global.hpp"

using namespace std;
using namespace chrono;

Global* gl;

int main() {
    // Create listening socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serv_addr;
    int len = sizeof(serv_addr);
    memset(&serv_addr, 0, sizeof(sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RIP_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr*)(&serv_addr), len);

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.9");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(int));

    gl = new Global;
    net_monitor();
    gl->start();

    sendRIP(RIP_REQUEST);

    RIP_Packet rip;
    sockaddr_in src_addr;
    while (true) {
        int recvlen = recvfrom(sockfd, rip.buf, BUFLEN, 0, (sockaddr*)(&src_addr), (socklen_t*)(&len));
        if (recvlen <= 0)
            continue;
        in_addr_t from = src_addr.sin_addr.s_addr;
        bool isSelf = false;
        for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next)
            if (ifc->addr.s_addr == from) {
                isSelf = true;
                break;
            }
        if (isSelf)
            continue;
        
        int tot = recvlen / 20;
        for (int i = 0; i < tot; ++i)
            rip.entry[i].reverseEndian();
        if (rip.cmd == RIP_REQUEST) {
            if (tot == 1 && rip.entry[0].afi == 0 && rip.entry[0].addr == 0x0 && rip.entry[0].metric == 16) {
                for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next)
                    if ((ifc->addr.s_addr & ifc->mask.s_addr) == (from & ifc->mask.s_addr)) {
                        sendRIP(RIP_RESPONSE, ifc);
                        break;
                    }
            }
            else
                printf("not supported\n");
        }
        else {
            lock_guard<mutex> rt_lg(gl->rt_mutex);
            for (int i = 0; i < tot; ++i) {
                rip.entry[i].metric += 1;
                if (rip.entry[i].metric > 16)
                    rip.entry[i].metric = 16;
                bool found = false;
                for (RT* rt = gl->rt_head; rt; rt = rt->next)
                    if (rt->addr == rip.entry[i].addr && rt->mask == rip.entry[i].mask) {
                        found = true;
                        if (rt->nextHop == rip.entry[i].nextHop || rt->metric > rip.entry[i].metric) {
                            rt->metric = rip.entry[i].metric;
                            rt->nextHop = from;
                            rt->expire = steady_clock::now() + seconds(30);
                        }
                        break;
                    }
                if (!found) {
                    RT* rt = new RT;
                    rt->addr = rip.entry[i].addr;
                    rt->mask = rip.entry[i].mask;
                    rt->metric = rip.entry[i].metric;
                    rt->nextHop = from;
                    rt->expire = steady_clock::now() + seconds(30);
                    rt->next = gl->rt_head;
                    gl->rt_head = rt;
                }
            }
        }
    }
    return 0;
}