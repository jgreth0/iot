#!/bin/bash

systemctl stop iot
useradd -m -d /var/iot/ -s /usr/sbin/nologin iot
mkdir -p /opt/iot/bin/
cp bin/* /opt/iot/bin/
touch /var/iot/iot.log
chown iot:iot /var/iot/iot.log
cp src/iot.service /etc/systemd/system/iot.service
systemctl daemon-reload
systemctl enable iot
systemctl start iot
