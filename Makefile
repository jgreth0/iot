CPPFLAGS=-std=c++17 -pthread -O3 -c
LDFLAGS=-std=c++17 -pthread -O5
PRESENCE_OBJ=obj/presence/presence.o

$(shell mkdir -p obj/kasa obj/presence obj/telemetry obj/home_assistant obj/module bin)

.PHONY: all

all: bin/iot bin/kasa_standalone bin/presence_standalone bin/telemetry_writer

bin/iot: obj/iot.o obj/presence/presence.o obj/kasa/kasa.o obj/kasa/kasa_io.o obj/telemetry/telemetry.o obj/module/module.o
	g++ $(LDFLAGS) -o $@ $^

bin/kasa_standalone: obj/kasa/kasa.o obj/kasa/kasa_io.o obj/kasa/kasa_standalone.o obj/module/module.o
	g++ $(LDFLAGS) -o $@ $^

bin/presence_standalone: obj/presence/presence.o obj/presence/presence_standalone.o obj/module/module.o
	g++ $(LDFLAGS) -o $@ $^

bin/telemetry_writer: obj/home_assistant/telemetry_writer.o obj/telemetry/telemetry.o obj/module/module.o
	g++ $(LDFLAGS) -o $@ $^


obj/iot.o: src/iot.cpp src/kasa/kasa.hpp src/presence/presence.hpp src/telemetry/telemetry.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/iot.cpp -o obj/iot.o

obj/kasa/kasa_standalone.o: src/kasa/kasa_standalone.cpp src/kasa/kasa.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/kasa/kasa_standalone.cpp -o obj/kasa/kasa_standalone.o

obj/kasa/kasa.o: src/kasa/kasa.cpp src/kasa/kasa.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/kasa/kasa.cpp -o obj/kasa/kasa.o

obj/kasa/kasa_io.o: src/kasa/kasa_io.cpp src/kasa/kasa.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/kasa/kasa_io.cpp -o obj/kasa/kasa_io.o

obj/presence/presence_standalone.o: src/presence/presence_standalone.cpp src/presence/presence.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/presence/presence_standalone.cpp -o obj/presence/presence_standalone.o

obj/presence/presence.o: src/presence/presence.cpp src/presence/presence.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/presence/presence.cpp -o obj/presence/presence.o

obj/home_assistant/telemetry_writer.o: src/home_assistant/telemetry_writer.cpp src/telemetry/telemetry.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/home_assistant/telemetry_writer.cpp -o obj/home_assistant/telemetry_writer.o

obj/telemetry/telemetry.o: src/telemetry/telemetry.cpp src/telemetry/telemetry.hpp src/module/module.hpp
	g++ $(CPPFLAGS) src/telemetry/telemetry.cpp -o obj/telemetry/telemetry.o

obj/module/module.o: src/module/module.cpp src/module/module.hpp
	g++ $(CPPFLAGS) src/module/module.cpp -o obj/module/module.o

clean:
	rm -rf obj bin
