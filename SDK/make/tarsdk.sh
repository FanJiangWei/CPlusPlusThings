#!/bin/bash

make clean
MYDATE=`date +%y_%m_%d-%H-%M`

project=ALL_SDK_ZC3780_$USER\_$MYDATE.tar.gz

echo $project

tar -zcvf ../../$project  --exclude=SDK/si --exclude=SDK/STD_2016bin --exclude=*.sisc --exclude=*\).c --exclude=*\).h ../../SDK
echo $project
