
# John Greth's IOT Utils

This utility is designed to be run 24/7 as a background process (or part of a
background process) so that changes in device state can be recorded as they
happen.

## KASA Utils

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