#!/bin/bash


if [ $1 == "total" ];
then
	file_name=$2"_total".csv
	if [ ! -f $file_name ];then
		echo "creat "$file_name""
		touch  $file_name
	else
		echo ""$file_name" has been exists!"
	fi
	echo $file_name > $file_name	#重置
	date >> $file_name
    readelf -s --wide $2 | awk '{x+=$3}{print}END{print x}' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2'  | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2'>> $file_name
    echo "total" >> $file_name
	
elif [ $1 == "sort" ];
then
	file_name=$2"_sort".csv
	if [ ! -f $file_name ];then
		echo "creat "$file_name""
		touch  $file_name
	else
		echo ""$file_name" has been exists!"
	fi
	echo $file_name > $file_name	#重置
	date >> $file_name
    readelf -s --wide $2 | awk '{print}' | sort --key=3,3 -g -r | awk '{x+=$3}{print}END{print x}' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2'  | sed 's/ \{1,6\}/,/2' | sed 's/ \{1,6\}/,/2'>> $file_name
    echo "total" >> $file_name
    #readelf -s $2 | awk '{print}' | sort --key=3,3 -g -r | awk '{x+=$3}{print}END{print x}'
    #echo "sort"
else
    echo "the arg#1 is $1"
fi
exit

