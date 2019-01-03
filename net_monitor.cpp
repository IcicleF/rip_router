#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

#include <cstring>
#include <string>
#include <iostream>

#include "net_monitor.hpp"
#include "global.hpp"

using namespace std;

extern int sockfd;

inline void freeIfList(Interface* head) {
    Interface* next;
    while (head) {
        next = head->next;
        delete head;
        head = next;
    }
}

bool net_monitor() {
    string base = "/sys/class/net/";
    ifaddrs* ifa;
    if (getifaddrs(&ifa) < 0)
        return false;
    FILE* fin;

    int ifCount = 0, newIfCount = 0, matches = 0;
    Interface* if_head = gl->if_head;
    while (if_head) {
        ++ifCount;
        if_head = if_head->next;
    }

    if_head = NULL;
    for (ifaddrs* cur = ifa; cur; cur = cur->ifa_next) {
        if (strcmp(cur->ifa_name, "lo") == 0 || strcmp(cur->ifa_name, "wlp4s0") == 0)
            continue;

        /* Carrier */
        string carrierName = base + cur->ifa_name + "/carrier";
        int carrier;
        fin = fopen(carrierName.c_str(), "r");
        if (fin) {
            fscanf(fin, "%d", &carrier);
            fclose(fin);
        }
        if (!carrier)
            continue;

        /* MAC Address */
        string addrName = base + cur->ifa_name + "/address";
        fin = fopen(addrName.c_str(), "r");
        int mac[8];
        fscanf(fin, "%02x:%02x:%02x:%02x:%02x:%02x", mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);
        fclose(fin);

        /* Interface Index */
        string ifindexName = base + cur->ifa_name + "/ifindex";
        fin = fopen(ifindexName.c_str(), "r");
        int ifIndex;
        fscanf(fin, "%d", &ifIndex);
        fclose(fin);

        /* IP Address & Subnet Mask */
        if (cur->ifa_addr == NULL)
            continue;
        in_addr ifAddr = ((sockaddr_in*)(cur->ifa_addr))->sin_addr;
        if (ifAddr.s_addr >> 8 == 0x0)
            continue;
        in_addr ifMask = (in_addr){ 0xFFFFFF };

        //printf("%s: ip=%s ", cur->ifa_name, inet_ntoa(ifAddr));
        //printf("mask=%s ", inet_ntoa(ifMask));
        //printf("mac=%02X%02X:%02X%02X:%02X%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        Interface* ifc = new Interface;
        ifc->name = cur->ifa_name;
        ifc->addr = ifAddr;
        ifc->mask = ifMask;
        ifc->index = ifIndex;
        memset(ifc->mac, 0, sizeof(ifc->mac));
        for (int i = 0; i < 6; ++i)
            ifc->mac[i] = (uint8_t)(mac[i] & 0xFF);
        ifc->next = if_head;
        ifc->sockfd = 0;
        if_head = ifc;

        Interface* p = gl->if_head;
        int match = 0;
        while (p) {
            if (*ifc == *p) {
                match = 1;
                break;
            }
            p = p->next;
        }
        matches += match;
        ++newIfCount;
    }
    bool ret = ifCount != newIfCount || matches != ifCount;
    if (ret) {
        for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next) {
            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.9");
            mreq.imr_interface.s_addr = ifc->addr.s_addr;
            setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
            close(ifc->sockfd);
        }
        freeIfList(gl->if_head);
        gl->if_head = if_head;
        int opt = 1;
        for (Interface* ifc = gl->if_head; ifc; ifc = ifc->next) {
            ifc->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            setsockopt(ifc->sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
            setsockopt(ifc->sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(int));

            ifreq ifr;
            memset(&ifr, 0, sizeof(ifreq));
            strncpy(ifr.ifr_name, ifc->name.c_str(), IF_NAMESIZE);
            setsockopt(ifc->sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifreq));

            sockaddr_in ifc_addr;
            memset(&ifc_addr, 0, sizeof(sockaddr_in));
            ifc_addr.sin_family = AF_INET;
            ifc_addr.sin_port = htons(RIP_PORT);
            bind(ifc->sockfd, (sockaddr*)(&ifc_addr), sizeof(sockaddr_in));

            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.9");
            mreq.imr_interface.s_addr = ifc->addr.s_addr;
            setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        }
    }
    else
        freeIfList(if_head);
    freeifaddrs(ifa);
    printf("\033[36m[NetMon]\033[0m %d interface(s)\n", newIfCount);
    return ret;
}