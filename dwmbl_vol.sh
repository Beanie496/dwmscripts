#!/bin/dash

muted=$(wpctl get-volume @DEFAULT_SINK@ | awk '{ print $3 }')
volume=$(wpctl get-volume @DEFAULT_SINK@ | awk '{ print $2 * 100 }')
if [ "$muted" = "[MUTED]" ]; then
	echo "🔇 $volume%"
        # this makes the spacing look nicer
	muted=" $muted"
elif [ $volume -lt 20 ]; then
	echo "🔈 $volume%"
elif [ $volume -lt 50 ]; then
	echo "🔉 $volume%"
else 
	echo "🔊 $volume%"
fi

dunstify -t 1000 -h string:x-dunst-stack-tag:speaker-volume-change "Speaker volume" "Volume: $volume% $muted"
