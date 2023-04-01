#!/bin/sh

battery="/sys/class/power_supply/BAT1"
cache="/tmp/batterynotifycache"
status=$(cat "$battery/status")
capacity=$(cat "$battery/capacity")
low=20
criticallylow=10
nearlyfull=90

case $status in
	"Full")
		notify-send -u low -t 5000 "Battery full" "Battery level at 100%"
		symbol="⚡"
		;;
	"Charging")
		if [ $capacity -ge $nearlyfull ]; then
			if [ "$(cat "$cache")" != "yes" ]; then
				dunstify -u low -t 3000 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery nearly full" "Battery level above $nearlyfull%"
				echo "yes" > "$cache"
			fi
		else
			echo "no" > "$cache"
		fi
		symbol="🔌"
		;;
	"Discharging")
		if [ $capacity -gt $criticallylow ]; then
			echo "no" > "$cache"
			symbol="🔋"
		elif [ $capacity -le $criticallylow ]; then
			dunstify -u critical -t 0 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery very low" "Battery level below $criticallylow%"
			symbol="❗"
		elif [ $capacity -le $low ]; then
			if [ "$(cat "$cache")" != "yes" ]; then
				dunstify -u normal -t 0 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery low" "Battery level below $low%"
			fi
			echo "yes" > "$cache"
			symbol="❗"
		fi
		;;
	*)
		exit 1;
		;;
esac

echo "$symbol $capacity%"
