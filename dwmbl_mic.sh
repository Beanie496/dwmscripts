#!/bin/dash

muted=$(wpctl get-volume @DEFAULT_SOURCE@ | awk '{ print $3 }')
volume=$(wpctl get-volume @DEFAULT_SOURCE@ | awk '{ print $2 * 100 }')
# wireplumber doesn't give a value for a few seconds after the volume is set
# initially, which happens when X is started for the first time when the
# computer boots
while [ "$volume" = "" ]; do
	sleep 0.05
	volume=$(wpctl get-volume @DEFAULT_SOURCE@ | awk '{ print $2 * 100 }')
done

if [ "$muted" != "" ]; then
	# this makes the spacing look nicer
	muted=" $muted"
fi
echo "🎤$muted $volume%"

dunstify -t 1000 -h string:x-dunst-stack-tag:mic-volume-change "Mic volume" "Volume: $volume%$muted"
