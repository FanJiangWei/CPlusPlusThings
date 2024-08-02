#!/bin/bash

if [ $1 == "total" ];
then
    readelf -s $2 | awk '{x+=$3}{print}END{print x}'
    echo "total"
elif [ $1 == "sort" ];
then
    readelf -s $2 | awk '{print}' | sort --key=3,3 -g -r | awk '{x+=$3}{print}END{print x}'
    echo "sort"
else
    echo "the arg#1 is $1"
fi
exit

