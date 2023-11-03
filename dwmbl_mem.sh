#!/bin/dash
free -m | awk '/^Mem/ { print "RAM: " $3 " / " $2 " MiB" }'
