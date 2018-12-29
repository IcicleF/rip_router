#if !defined(GLOBAL_HPP)
#define GLOBAL_HPP

#include "interface.hpp"
#include "rt.hpp"
#include "watcher.hpp"
#include "eth_monitor.hpp"

#include <thread>
#include <mutex>

struct Global {
    RT* rt_head;
    Interface* if_head;

    bool rt_modified;

    std::mutex rt_mutex;

    Global() : rt_head(NULL), if_head(NULL), rt_modified(false) { }
    void start() {
        std::thread(watcher).detach();
        std::thread(eth_monitor).detach();
    }
};

extern Global* gl;

#endif // GLOBAL_HPP
