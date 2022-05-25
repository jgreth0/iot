CPPFLAGS=-std=c++17 -pthread -O3 -c
LDFLAGS=-std=c++17 -pthread -O5
KASA_OBJ=obj/kasa/kasa.o obj/kasa/kasa_io.o obj/kasa/kasa_helpers.o

$(shell mkdir -p obj/kasa bin)

.PHONY: all

all: bin/kasa_standalone

bin/kasa_standalone: $(KASA_OBJ) obj/kasa/kasa_standalone.o
	g++ $(LDFLAGS) -o $@ $^

obj/kasa/%.o: src/kasa/%.cpp src/kasa/kasa.hpp
	g++ $(CPPFLAGS) src/kasa/$*.cpp -o obj/kasa/$*.o

clean:
	rm -rf obj bin
