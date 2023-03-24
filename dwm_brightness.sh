#!/bin/sh
max=$(brightnessctl m)
current=$(brightnessctl g)
percent=$(( $current * 100 / $max))
dunstify -t 1000 -h string:x-dunst-stack-tag:screen-brightness-change "Brightness" "New brightness: $percent%\n($current / $max)"
