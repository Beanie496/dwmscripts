#!/bin/sh

muted=$(wpctl get-volume @DEFAULT_SOURCE@ | awk '{ print $3 }')
volume=$(wpctl get-volume @DEFAULT_SOURCE@ | awk '{ print $2 * 100 }')

if [ "$muted" != "" ]; then
	# this makes the spacing look nicer
	muted=" $muted"
fi
echo "🎤$muted $volume%"

dunstify -t 1000 -h string:x-dunst-stack-tag:mic-volume-change "Mic volume" "Volume: $volume%$muted"
