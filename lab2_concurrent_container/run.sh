#!/bin/sh

dir=/data
id=$(head -c 4 /dev/urandom | od -An -tx1 | tr -d ' \n')
count=0

while true; do
	count=$((count + 1))

	{
		flock 9
		i=1
		while [ -e "$dir/$(printf '%03d' "$i")" ]; do
			i=$((i + 1))
		done
		name=$dir/$(printf '%03d' "$i")
		echo "$id $count" >"$name"
	} 9>"$dir/.lock"

	sleep 1
	rm -f "$name"
	sleep 1
done
