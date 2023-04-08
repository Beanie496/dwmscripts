#!/bin/sh

battery="/sys/class/power_supply/BAT1"
cache="/tmp/batterynotifycache"
status=$(cat "$battery/status")
capacity=$(cat "$battery/capacity")
nearlyfull=90
low=20
criticallylow=10
# this is used to set the cache to a battery level within normal levels.
# this allows the program to know when to notify
normal=$(($nearlyfull - 1))

case $status in
	"Full")
		notify-send -u low -t 5000 "Battery full" "Battery level at 100%"
		symbol="⚡"
		;;
	"Charging")
		# if the battery is plugged in at 5%, the script runs, then the battery is unplugged,
		# a notification should be sent. Setting the cache to a normal level tells this script
		# to notify if the battery is unplugged at 5% for some reason.
		if [ $capacity -ge $nearlyfull ]; then
			if [ "$(cat "$cache")" -lt $nearlyfull ]; then
				dunstify -u low -t 3000 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery nearly full" "Battery level above $nearlyfull%"
				echo "$nearlyfull" > "$cache"
			fi
		else
			echo "$normal" > "$cache"
		fi
		symbol="🔌"
		;;
	"Discharging")
		# same deal. if it's unplugged and replugged at 95%, the program should still notify for
		# a nearly-full battery.
		if [ $capacity -le $criticallylow ]; then
			if [ "$(cat "$cache")" -gt $criticallylow ]; then
				dunstify -u critical -t 5000 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery very low" "Battery level below $criticallylow%"
				echo "$criticallylow" > "$cache"
			fi
			symbol="❗"
		elif [ $capacity -le $low ]; then
			if [ "$(cat "$cache")" -gt "$low" ]; then
				dunstify -u normal -t 5000 -h string:x-dunst-stack-tag:battery-percentage-alert "Battery low" "Battery level below $low%"
				echo "$low" > "$cache"
			fi
			symbol="❗"
		else
			echo "$normal" > "$cache"
			symbol="🔋"

		fi
		;;
	*)
		exit 1;
		;;
esac

echo "$symbol $capacity%"
