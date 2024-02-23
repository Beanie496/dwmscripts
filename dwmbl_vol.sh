#!/bin/dash

muted=$(wpctl get-volume @DEFAULT_SINK@ | awk '{ print $3 }')
volume=$(wpctl get-volume @DEFAULT_SINK@ | awk '{ print $2 * 100 }')
# wireplumber doesn't give a value for a few seconds after the volume is set
# initially, which happens when X is started for the first time when the
# computer boots
while [ "$volume" = "" ]; do
	sleep 0.05
	volume=$(wpctl get-volume @DEFAULT_SINK@ | awk '{ print $2 * 100 }')
done

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

dunstify -t 1000 -h string:x-dunst-stack-tag:speaker-volume-change "Speaker volume" "Volume: $volume%$muted"
