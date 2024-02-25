#!/bin/bash

logger MIRG: starting run script
while true
do 
    midi_dev=$(amidi -l | grep "OP-1 MIDI 1" | awk '{print $2}')
    if [ "$midi_dev" ]
    then
        nice -n-20 /home/pi/mirg/mirg $midi_dev
    fi
    sleep 1
done
