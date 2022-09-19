
# John Greth's IOT Utils

This utility runs in the background and coordinates the functionality of home
internet-of-things devices. Some features include:

- (MISSING) Turning on a dehumidifier when outside humidity is high
- (DONE) Turning on an air filter at specified times, but not when the TV is on
(to avoid the noise).
- (MISSING) Turning on outdoor lights when the sun goes down and turning them
off once everyone is home.
- (DONE) Logging when devices turn on and off to estimate power usage.

These features rely on underlying capabilities:

- (DONE) Controlling and checking the state of smart plugs and switches.
- (PARTIAL) Detecting the presence of the TV and everyone's cell phones on the
network.
- (MISSING) Fetching weather condition and forecast from noaa (weather.gov)
including temperature, humidity, and sunset/sunrise times.

This utility is designed to be run 24/7 as a background process (or part of a
background process) so that changes in device state can be recorded as they
happen.

In the current state, the server starts to misbehave if it runs continuously for
over a month. There must be some kind of leak but it is difficult to debug
errors that happen so infrequently.

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

- Implement the missing features above.
- Add additional error catching and reporting to isolate instability issues.
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
