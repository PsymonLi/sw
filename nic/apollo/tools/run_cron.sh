#!/bin/bash

date > /tmp/cron_test

# free up some space in /var/log/pensando/ by stashing
# rotated logs out to /data/logstash/
mkdir -p /data/logstash/
mv /var/log/pensando/*.log.* /data/logstash/
