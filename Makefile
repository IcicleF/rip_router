.PHONY: clean

CXXFLAGS = -O2 -std=c++11 -Wno-unused-result
LINKERS = -lpthread

HEADERS := $(wildcard *.hpp)
CXXFILES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(CXXFILES))

main: $(OBJECTS) $(HEADERS)
	g++ $(CXXFLAGS) -o daemon $(OBJECTS) $(LINKERS)

main.o: main.cpp global.hpp
eth_monitor.o: eth_monitor.cpp eth_monitor.hpp global.hpp
net_monitor.o: net_monitor.cpp net_monitor.hpp global.hpp
rt_monitor.o: rt_monitor.cpp rt_monitor.hpp global.hpp
sender.o: sender.cpp sender.hpp global.hpp
watcher.o: watcher.cpp watcher.hpp global.hpp

clean:
	rm daemon *.o