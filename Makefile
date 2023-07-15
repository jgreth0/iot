CPPFLAGS=-std=c++17 -g -rdynamic -pthread -O0 -c
LDFLAGS= -std=c++17 -g -rdynamic -pthread -O0
MODULE_OBJ= \
    obj/modules/kasa.o \
    obj/modules/unit.o \
    obj/modules/module.o \
    obj/modules/icmp_helper.o \
    obj/modules/presence_wrap.o \
    obj/modules/presence_shmem.o \
    obj/modules/shmem.o \
    obj/modules/signal_handler.o \
    obj/modules/json_fetcher.o

$(shell mkdir -p obj/modules bin)

.PHONY: all clean

.SECONDARY:

all: bin/sandbox bin/iot bin/kasa_standalone bin/kasa_testbench bin/presence_standalone

obj/%.o: src/%.cpp src/modules/*.hpp
	g++ $(CPPFLAGS) src/$*.cpp -o $@

obj/iot.o: src/iot.cpp src/automations/*.hpp src/modules/*.hpp
	g++ $(CPPFLAGS) src/iot.cpp -o $@

bin/%: obj/%.o $(MODULE_OBJ)
	g++ $(LDFLAGS) -o $@ $^ -lcurl -ljsoncpp

clean:
	rm -rf obj bin
