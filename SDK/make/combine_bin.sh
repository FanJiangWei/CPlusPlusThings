#!/bin/bash
export LC_ALL=C.UTF-8
export LANG=C.UTF-8

bin_path='../STD_2016bin/'
com_Path="../STD_2016bin/combine/" 
config_Path="../config_bin/" 

# 定义打印绿色文本的函数
print_green() 
{
    echo -e "\033[1;32m$1\033[0m"
}

check_config_folder() 
{
    # 检查文件夹是否存在且为空
    if [ ! -d "$config_Path" ] || [ -z "$(ls -A $config_Path/*.bin 2>/dev/null)" ]; then
        print_green "配置文件不存在或文件夹为空!!!"
        bash ./make_config_bin.sh "2" "$1"
    else
        # 循环遍历所有 .bin 文件进行检查
        for file in "$config_Path"/*.bin; do
            # 使用正则表达式提取文件名中的省份代码
            if [[ $file =~ ([0-9]+) ]]; then
                province_code=${BASH_REMATCH[1]}

                # 比较提取的省份代码和传入的 $1
                if [ "$province_code" != "$1" ]; then
                    print_green "文件名无效: $file"
                    rm -f "$config_Path"/*.bin
                    bash ./make_config_bin.sh "2" "$1"
                    break  # 出现不一致立即中断遍历
                fi
            else
                print_green "无法从文件名提取省份代码: $file"
                rm -f "$config_Path"/*.bin
                bash ./make_config_bin.sh "2" "$1"
                break  # 出现不一致立即中断遍历
            fi
        done
    fi
}

	
cco_symbol="ZCDBU"
sta_symbol="ZCDBM"
cjq_symbol="ZCDBC"

combine_data_bin_and_config_bin()
{
	src_file_name=$1
	d_type=$2
     p_code=$(awk 'BEGIN{printf("%02d",'$3')}')
	
	if [ ! -d "$com_Path" ]; then  
		mkdir "$com_Path"
	fi	
	
	if [ "$d_type" == "2" ];then
		if [[ "$src_file_name" == *"ZCDBU"* ]];then
			d_name="cco_233l"
		elif [[ "$src_file_name" == *$sta_symbol* ]];then
			d_name="sta_233l"
		elif [[ "$src_file_name" == *$cjq_symbol* ]];then
			d_name="cjq2_233l"
		else
			echo "param 1 error1!!!"
		fi
	elif [ "$d_type" == "d" ] || [ "$d_type" == "D" ];then
		if [[ "$src_file_name" == *$cco_symbol* ]];then
			d_name="cco_venus"
		elif [[ "$src_file_name" == *$sta_symbol* ]];then
			d_name="sta_venus"
		elif [[ "$src_file_name" == *$cjq_symbol* ]];then
			d_name="cjq2_venus"
		else
			echo "param 1 error2!!!"
		fi
	elif [ "$d_type" == "v" ] || [ "$d_type" == "V" ];then
		if [[ "$src_file_name" == *$cco_symbol* ]];then
			d_name="cco_riscv"
		elif [[ "$src_file_name" == *$sta_symbol* ]];then
			d_name="sta_riscv"
		elif [[ "$src_file_name" == *$cjq_symbol* ]];then
			d_name="cjq2_riscv"
		else
			echo "param 1 error3!!!"
		fi
	else
		echo "param 2 error!!!"
		exit
	fi
	
	echo $src_file_name $d_type $d_name $3 $p_code
	
	for file in $config_Path*
	do
         #省份配置Bin文件
		tmp_f=$(basename "$file")
		if [[ $tmp_f == *$d_name* ]];then
              #查找编译目标文件名中的省份代码-".xx-",字符串从18开始取4个字符
              src_p_code=`echo ${src_file_name:18:4}`
              #提取省份配置Bin文件名中省份代码
			province_code=`echo $tmp_f | cut -d '_' -f3`
             if [ $p_code == 99 ];then
                    #将文件名中的".00-"省份代码替换为配置文件名中的省份代码
                    #new_file_name=`echo ${src_file_name/".00-"/"."$province_code"-"}`
                    new_file_name=`echo ${src_file_name/"$src_p_code"/"."$province_code"-"}`
                    cat $src_file_name $config_Path$tmp_f > $com_Path$new_file_name
                    echo -e $tmp_f"\t"$province_code"\t"$new_file_name
             else
                if [ "$p_code" == "$province_code" ];then
                    #将文件名中的".00-"省份代码替换为配置文件名中的省份代码
                    #new_file_name=`echo ${src_file_name/".00-"/"."$province_code"-"}`
                    # new_file_name=`echo ${src_file_name/"$src_p_code"/"."$province_code"-"}`
                    cat $src_file_name $config_Path$tmp_f > $com_Path$src_file_name
                    # echo -e $tmp_f"\t"$province_code"\t"$new_file_name
                fi
             fi
		fi
	done
}	
