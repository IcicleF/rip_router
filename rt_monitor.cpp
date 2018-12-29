#include "rt_monitor.hpp"
#include "global.hpp"

bool rt_monitor(const TimePoint now) {
    RT* prev = NULL;
    bool res = false;
    for (RT* cur = gl->rt_head; cur; cur = cur->next)
        if (now > cur->expire) {
            if (prev)
                prev->next = cur->next;
            else
                gl->rt_head = cur->next;
            delete cur;
            res = true;
        }
        else
            prev = cur;
    return res;        
}