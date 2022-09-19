CPPFLAGS=-std=c++17 -g -rdynamic -pthread -O0 -c
LDFLAGS= -std=c++17 -g -rdynamic -pthread -O0
OBJ= \
	obj/kasa.o \
	obj/module.o \
	obj/presence_tcp.o \
	obj/presence_icmp.o \
	obj/signal_handler.o \
	obj/shmem.o

$(shell mkdir -p obj bin)

.PHONY: all

all: bin/iot bin/kasa_standalone bin/presence_standalone bin/home_assistant

bin/iot: obj/iot.o $(OBJ)
	g++ $(LDFLAGS) -o $@ $^

bin/kasa_standalone: obj/kasa_standalone.o $(OBJ)
	g++ $(LDFLAGS) -o $@ $^

bin/presence_standalone: obj/presence_standalone.o $(OBJ)
	g++ $(LDFLAGS) -o $@ $^

bin/home_assistant: obj/home_assistant.o $(OBJ)
	g++ $(LDFLAGS) -o $@ $^

obj/%.o: src/%.cpp src/*.hpp Makefile
	g++ $(CPPFLAGS) src/$*.cpp -o obj/$*.o

clean:
	rm -rf obj bin
