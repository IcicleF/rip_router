#include <random>

#include "watcher.hpp"
#include "net_monitor.hpp"
#include "rt_monitor.hpp"
#include "sender.hpp"
#include "global.hpp"

using namespace std;
using namespace chrono;

void watcher() {
    TimePoint tCheckIf = steady_clock::now() + seconds(3);
    TimePoint tBroadcastRT = steady_clock::now() + seconds(5);
    random_device rd;
    while (true) {
        TimePoint now = steady_clock::now();
        bool rt_modified = false, if_modified = false;
        {
            lock_guard<mutex> rt_lg(gl->rt_mutex);
            rt_modified = rt_monitor(now);
        }
        if (now > tCheckIf) {
            if_modified = net_monitor();
            tCheckIf = steady_clock::now() + seconds(3);
        }
        if (gl->rt_modified || rt_modified || if_modified || now > tBroadcastRT) {
            sendRIP(RIP_RESPONSE);
            {
                lock_guard<mutex> rt_lg(gl->rt_mutex);
                gl->rt_modified = false;
            }
            tBroadcastRT = steady_clock::now() + milliseconds(4500 + rd() % 1000);
        }
        // todo: sync with forward
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}