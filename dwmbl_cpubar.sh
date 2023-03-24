#!/bin/sh
# get overall cpu use
overallcache="/tmp/cpucache"
corecache="/tmp/cpubarcache"

tickrate=$(getconf CLK_TCK)
cores=$(("$(awk 'END { print NR }' "/proc/stat")" - 8))

overallstats=$(awk '{ printf "%.2f" $1 }' /proc/uptime)
corestats=$(awk '/cpu[0-9]+/ { printf "%d ", $5 }' /proc/stat)
# If the cached file doesn't exist, copy the contents of stats and exit
[ ! -f $cache ] && echo "$stats" > "$cache" && exit 0
oldstats=$(cat "$cache")
elapsedtime=$( echo "$(echo "$stats" | awk '{ print $1 }') - $(echo "$oldstats" | awk '{ print $1 }')" | bc)
start=$(echo "$oldstats" | awk '{ print $2 }')
end=$(echo "$stats" | awk '{ print $2 }')
difference=$(($end - $start))
loadavg=$(echo "$difference / $cores / $tickrate / $elapsedtime)" | bc)
echo $loadavg

#	"0") printf " ";;
#	"1") printf "▁";;
#	"2") printf "▂";;
#	"3") printf "▃";;
#	"4") printf "▄";;
#	"5") printf "▅";;
#	"6") printf "▆";;
#	"7") printf "▇";;
#	"8") printf "█";;
#	*)   printf "error";;
echo "$stats" > "$cache"
