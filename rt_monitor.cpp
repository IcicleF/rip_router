#include "rt_monitor.hpp"
#include "global.hpp"

int counter = 0;
bool rt_monitor(const TimePoint now) {
    bool res = false;
    ++counter;
    if (counter == 20)
        counter = 0;
    if (counter == 1)
        printf("[RtMon]\n");
    for (RT* cur = gl->rt_head; cur; cur = cur->next) {
        if (now > cur->expire)
            cur->metric = 16;
        if (counter == 1) {
            printf("  %s ", inet_ntoa((in_addr){cur->addr}));
            printf("mask=%s ", inet_ntoa((in_addr){cur->mask}));
            printf("hop=%s ", inet_ntoa((in_addr){cur->nextHop}));
            printf("metric=%d\n", cur->metric);
        }
    }
    return res;        
}