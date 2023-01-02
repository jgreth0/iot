
# John Greth's IOT Utils

This utility runs in the background and coordinates the functionality of home
internet-of-things devices. Much of the configuration is hard coded in C++. This
repo should be taken as a starting point or as a set of examples and not as a
complete/polished solution. Some features include:

- (DONE) Turning on an air filter at specified times.
- (DONE) Turning on outdoor lights in the evening.
- (DONE) Logging when devices turn on and off to estimate power usage.
- (DONE) Simulate the linkage between a smart plug and a smart switch.
- (MISSING) Turning on a dehumidifier when outside humidity is high.

These features rely on underlying capabilities:

- (DONE) Controlling and checking the state of smart plugs and switches.
- (PARTIAL) Detecting the presence of devices on the network.
- (MISSING) Fetching weather condition and forecast from noaa (weather.gov)
including temperature, humidity, and sunset/sunrise times.

This utility is designed to be run 24/7 as a background process (or part of a
background process) so that changes in device state can be recorded as they
happen.

## Installation

```sh
make -j4
sudo ./install.sh
```

- Installs the binaries in /opt/iot/ .
- creates a user named "iot".
- creates /var/iot/iot.log which is owned by the new user.
- creates a service which runs the utility as the new user on startup.

## TODO list

- Fetch and use weather data from the weather.gov api.
- Add presence detection by querying the lease table of a kea dhcp server.
- Add arp table refresher to keep the router's active client list up to date.

## Standalone Utilities

### KASA Utils

The KASA utility facilitates controlling and logging the state of tp-link kasa
smart plugs and switches. I've tested it with the HS103 smart plug and I believe
it should work with hs100, hs110, hs200, KP105 and possibly other devices.

This utility is multi-threaded so that TCP connections can be open to many
devices simultaneously. This avoids any performance issues in case the number of
devices is large. Also, function calls are non-blocking unless otherwise stated.

A bash utility for switching and checking tp-link kasa devices is available from
https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug .
This may be a more convenient option for scripting or controlling devices with
crontab. I reverse engineered George's code to determine much of the needed IO
functionality.

```sh
make -j4
./bin/kasa_standalone -h # usage
./bin/kasa_standalone -v 5 -l kasa.log -c kasa.conf -i
```

### KASA Testbench

This utility establishes a connection to a kasa device and enables raw CLI
access to that device. User input (json format) is encoded and sent to the
device. Responses from the device are decoded and printed to the terminal. This
is useful for testing out new device capabilities before adding them to the kasa
module.

```sh
make -j4
./bin/kasa_testbench -h # usage
```

### Presence Standalone

Sets up a presence_icmp module to periodically ping the target network device to
determine if it is connected to the network.

```sh
make -j4
./bin/presence_standalone -h # usage
```
