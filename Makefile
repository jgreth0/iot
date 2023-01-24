CPPFLAGS=-std=c++17 -g -rdynamic -pthread -O0 -c
LDFLAGS= -std=c++17 -g -rdynamic -pthread -O0 -lcurl -ljsoncpp
MODULE_OBJ= \
    obj/modules/kasa.o \
    obj/modules/module.o \
    obj/modules/presence_icmp.o \
    obj/modules/presence_wrap.o \
    obj/modules/presence_shmem.o \
    obj/modules/shmem.o \
    obj/modules/signal_handler.o

$(shell mkdir -p obj/modules bin)

.PHONY: all clean

.SECONDARY:

all: bin/sandbox bin/iot bin/kasa_standalone bin/kasa_testbench bin/presence_standalone

obj/%.o: src/%.cpp src/modules/*.hpp Makefile
	g++ $(CPPFLAGS) src/$*.cpp -o obj/$*.o

obj/iot.o: src/iot.cpp src/automations/*.hpp src/modules/*.hpp Makefile
	g++ $(CPPFLAGS) src/iot.cpp -o obj/iot.o

bin/%: obj/%.o $(MODULE_OBJ)
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -rf obj bin
