#!/bin/sh

muted=$(pactl get-sink-mute @DEFAULT_SINK@ | awk '{ print $2 }')
volume=$(pactl get-sink-volume @DEFAULT_SINK@ | awk '{ if(NR == 1) print ($5 + $12) / 2 }')
if [ "$muted" = "yes" ]; then
	echo "🔇 $volume%"
elif [ $volume -lt 20 ]; then
	echo "🔈 $volume%"
elif [ $volume -lt 50 ]; then
	echo "🔉 $volume%"
else 
	echo "🔊 $volume%"
fi

dunstify -t 1000 -h string:x-dunst-stack-tag:speaker-volume-change "Speaker volume" "Muted: $muted\nVolume: $volume%"
