
config_Path="../config_bin/" 
	if [ ! -d "$config_Path" ]; then  
		mkdir "$config_Path"
	fi


	standard_year_10=`date +%-y`
	standard_month_10=`date +%-m`
	standard_day_10=`date +%-d`
	standard_hour_10=`date +%-H`
	standard_min_10=`date +%-M`
	standard_sec_10=`date +%-S`
	standard_year=$(awk 'BEGIN{printf("0x%02d",'$standard_year_10')}')
	standard_month=$(awk 'BEGIN{printf("0x%02d",'$standard_month_10')}')
	standard_day=$(awk 'BEGIN{printf("0x%02d",'$standard_day_10')}')
	standard_hour=$(awk 'BEGIN{printf("0x%02d",'$standard_hour_10')}')
	standard_min=$(awk 'BEGIN{printf("0x%02d",'$standard_min_10')}')
	standard_sec=$(awk 'BEGIN{printf("0x%02d",'$standard_sec_10')}')
	
	fn_year=$(awk 'BEGIN{printf("%02d",'$standard_year_10')}')
	fn_month=$(awk 'BEGIN{printf("%02d",'$standard_month_10')}')
	fn_day=$(awk 'BEGIN{printf("%02d",'$standard_day_10')}')
	fn_hour=$(awk 'BEGIN{printf("%02d",'$standard_hour_10')}')
	fn_min=$(awk 'BEGIN{printf("%02d",'$standard_min_10')}')
	fn_sec=$(awk 'BEGIN{printf("%02d",'$standard_sec_10')}')
	
#选择制作方式
if [ -z "$1" ]; then
    read -p "Please choose make type: 1(make by device type), 2(make by province): " m_type
else
    m_type="$1"
fi

declare -i m_type

if [ "$m_type" == "1" ];then
	#选择需要编译的image�? u-> unicorn   r->roland  z->zc3 d->venus
	read -p "Please input your Device,z(3750), u(2M) r(1M) d(venus) 2(233l) v(riscv):" d_type
	declare -i d_type

	if [ "$d_type" == "z" ];then
		echo "make zc"
		choose_type="t_3750"
	elif [ "$d_type" == "u" ];then
		echo "make unicorn"
		choose_type="t_3750A"
	elif [ "$d_type" == "r" ];then
		echo "make roland"

	elif [ "$d_type" == "d" ];then
		echo "make venus"
		choose_type="t_venus"
	elif [ "$d_type" == "2" ];then
		echo "make 233l"
		choose_type="t_233l"
	elif [ "$d_type" == "v" ];then
		echo "make riscv"
		choose_type="t_riscv"
	else
		echo "unkonw d_type"
		exit
	fi
elif [ "$m_type" == "2" ]; then
    # 配置bin省份：0-仅生成国网基础版本配置Bin；1~34：生成输入的省份配置bin
    if [ -z "$2" ]; then
        read -p "please input province code: 0(gw), 1~34(province):" province_in
    else
        province_in="$2"
    fi

    # 去掉字符串province_in中的前导零，确保其被正确解释为十进制整数
    province_in=$(echo "$province_in" | sed 's/^0*//')

    # 处理输入为空的情况
    if [ -z "$province_in" ]; then
        province_in=0
    fi

    if ! [[ "$province_in" =~ ^[0-9]+$ ]]; then
        echo "not integer!!!"
        exit
    fi

    if [[ $province_in -lt 0 || $province_in -gt 34 ]]; then
        echo "unknown province code!!!"
        exit
    fi

    tmp_p=$(awk 'BEGIN{printf("%02d",'$province_in')}')

    # 十进制转换为BCD
    let "dst_province=(((10#$tmp_p / 10) << 4) + (10#$tmp_p % 10))"
else
    echo "You don't input make type!!"
    exit
fi




config_file_name="province_config_table.csv"

#定义名称数组
in_ver_name_array=(sta_3750_version 		sta_3750A_version 		sta_venus_version 		sta_riscv_version 		sta_233l_version
				   cjq2_3750_version 		cjq2_3750A_version 		cjq2_venus_version 		cjq2_riscv_version 		cjq2_233l_version
				   cco_3951_version 		cco_3951A_version 		cco_venus_version 		cco_riscv_version 		cco_233l_version)
out_ver_name_array=(sta_3750_out_version 	sta_3750A_out_version 	sta_venus_out_version 	sta_riscv_out_version	sta_233l_out_version
					cjq2_3750_out_version 	cjq2_3750A_out_version 	cjq2_venus_out_version 	cjq2_riscv_out_version 	cjq2_233l_out_version
					cco_3951_out_version 	cco_3951A_out_version 	cco_venus_out_version 	cco_riscv_out_version 	cco_233l_out_version)

#声明关联数组
declare -A in_ver_array
declare -A out_ver_array

total_col=$(awk -F, 'NR==1{print NF;exit}' $config_file_name)
echo "总列�?:"$total_col

#function get_crc16_value
function get_fcstab_value
{
     fcstab=(16#0000 16#1189 16#2312 16#329b 16#4624 16#57ad 16#6536 16#74bf\
             16#8c48 16#9dc1 16#af5a 16#bed3 16#ca6c 16#dbe5 16#e97e 16#f8f7\
             16#1081 16#0108 16#3393 16#221a 16#56a5 16#472c 16#75b7 16#643e\
             16#9cc9 16#8d40 16#bfdb 16#ae52 16#daed 16#cb64 16#f9ff 16#e876\
             16#2102 16#308b 16#0210 16#1399 16#6726 16#76af 16#4434 16#55bd\
             16#ad4a 16#bcc3 16#8e58 16#9fd1 16#eb6e 16#fae7 16#c87c 16#d9f5\
             16#3183 16#200a 16#1291 16#0318 16#77a7 16#662e 16#54b5 16#453c\
             16#bdcb 16#ac42 16#9ed9 16#8f50 16#fbef 16#ea66 16#d8fd 16#c974\
             16#4204 16#538d 16#6116 16#709f 16#0420 16#15a9 16#2732 16#36bb\
             16#ce4c 16#dfc5 16#ed5e 16#fcd7 16#8868 16#99e1 16#ab7a 16#baf3\
             16#5285 16#430c 16#7197 16#601e 16#14a1 16#0528 16#37b3 16#263a\
             16#decd 16#cf44 16#fddf 16#ec56 16#98e9 16#8960 16#bbfb 16#aa72\
             16#6306 16#728f 16#4014 16#519d 16#2522 16#34ab 16#0630 16#17b9\
             16#ef4e 16#fec7 16#cc5c 16#ddd5 16#a96a 16#b8e3 16#8a78 16#9bf1\
             16#7387 16#620e 16#5095 16#411c 16#35a3 16#242a 16#16b1 16#0738\
             16#ffcf 16#ee46 16#dcdd 16#cd54 16#b9eb 16#a862 16#9af9 16#8b70\
             16#8408 16#9581 16#a71a 16#b693 16#c22c 16#d3a5 16#e13e 16#f0b7\
             16#0840 16#19c9 16#2b52 16#3adb 16#4e64 16#5fed 16#6d76 16#7cff\
             16#9489 16#8500 16#b79b 16#a612 16#d2ad 16#c324 16#f1bf 16#e036\
             16#18c1 16#0948 16#3bd3 16#2a5a 16#5ee5 16#4f6c 16#7df7 16#6c7e\
             16#a50a 16#b483 16#8618 16#9791 16#e32e 16#f2a7 16#c03c 16#d1b5\
             16#2942 16#38cb 16#0a50 16#1bd9 16#6f66 16#7eef 16#4c74 16#5dfd\
             16#b58b 16#a402 16#9699 16#8710 16#f3af 16#e226 16#d0bd 16#c134\
             16#39c3 16#284a 16#1ad1 16#0b58 16#7fe7 16#6e6e 16#5cf5 16#4d7c\
             16#c60c 16#d785 16#e51e 16#f497 16#8028 16#91a1 16#a33a 16#b2b3\
             16#4a44 16#5bcd 16#6956 16#78df 16#0c60 16#1de9 16#2f72 16#3efb\
             16#d68d 16#c704 16#f59f 16#e416 16#90a9 16#8120 16#b3bb 16#a232\
             16#5ac5 16#4b4c 16#79d7 16#685e 16#1ce1 16#0d68 16#3ff3 16#2e7a\
             16#e70e 16#f687 16#c41c 16#d595 16#a12a 16#b0a3 16#8238 16#93b1\
             16#6b46 16#7acf 16#4854 16#59dd 16#2d62 16#3ceb 16#0e70 16#1ff9\
             16#f78f 16#e606 16#d49d 16#c514 16#b1ab 16#a022 16#92b9 16#8330\
             16#7bc7 16#6a4e 16#58d5 16#495c 16#3de3 16#2c6a 16#1ef1 16#0f78)

            let "fcstab_index = $RestCRC16Value ^ $1 "
            let "fcstab_index = $fcstab_index & 16#ff "
            let "fcstab_value= ${fcstab[fcstab_index]}"        
}

get_crc16_value()
{
	crc_data=($*)
	:<<!
	echo "len: " ${#crc_data[@]}
	for key in ${!crc_data[*]}
		do
			printf "%02x " ${crc_data[$key]}
		done
	printf "\n"
!
	
	let "len=${#crc_data[*]}"
	let "RestCRC16Value=16#ffff"
	let "crc16_value_1= $RestCRC16Value >> 8"

	for((i=0;i<$len;i++))
    do 
		data=$(awk 'BEGIN{printf("0x%02x",'${crc_data[i]}')}')
		get_fcstab_value $data
		let "CRC16Value = $crc16_value_1 ^ $fcstab_value"
		let "RestCRC16Value = CRC16Value"
		let "crc16_value_1= $CRC16Value >> 8"	
    done

    let "CRC16Value = ~$CRC16Value"
    let "CRC16Value = $CRC16Value & 16#ffff"
    CRC16Value=$(awk 'BEGIN{printf("0x%04x",'$CRC16Value')}')
    echo "CRC16Value:"$CRC16Value
}

#oldIFS=$IFS
#IFS=,

#total_col=3

find_province=0
bin_array=()
for((ii=0; ii < (total_col - 1);ii++))
do
	let "cur_col=ii+2"	
	echo "cur col is:"$cur_col
	func_switch=0x00000000
	func_switch_res1=0x00000000
	func_switch_res2=0x0000
	
	param_cfg_res_array=(0 0 0 0 0 0 0 0 0 0 0)
	#初�?�化bin数组
	unset bin_array
	idx=2
	
	while read -r line
	do
		#echo line $i:$line
		seg_name=""
		seg_value=""
		seg_name=`echo $line | cut -d ',' -f1`
		seg_value=`echo $line | cut -d ',' -f$cur_col`
		
		#echo "seg_name : " ${#seg_name}
		#echo "seg_value: " ${#seg_value}
		
		if [ -z "$seg_name" ] && [ -z "$seg_value" ];then
			echo "gap line!!!!!!!!!!!!!!"
			continue	
		fi
		
		if [ "" != "$seg_name" ] && [ "" == "$seg_value" ];then
			seg_value=0x0000
			echo "seg_value is null, set: " $seg_value
		fi
		
		if [ "province_name" == "$seg_name" ];then
			province_name=$seg_value
			echo "province:"$province_name
			continue
		fi
		
		if [ "ex_info_len" == "$seg_name" ];then
			let "ex_info_len_h=seg_value >> 8 & 16#ff"
			let "ex_info_len_l=seg_value & 16#ff"
			let bin_array[idx]=ex_info_len_l idx++
			let bin_array[idx]=ex_info_len_h idx++
			printf "ex_info_len: %02x%02x\n" $ex_info_len_h $ex_info_len_l
			continue
		fi
		
		if [ "water_mark" == "$seg_name" ];then
			let "water_mark_1=seg_value & 16#ff"
			let "water_mark_2=seg_value >> 8 & 16#ff"
			let "water_mark_3=seg_value >> 16 & 16#ff"
			let "water_mark_4=seg_value >> 24 & 16#ff"
			
			let bin_array[idx]=water_mark_1 idx++
			let bin_array[idx]=water_mark_2 idx++
			let bin_array[idx]=water_mark_3 idx++
			let bin_array[idx]=water_mark_4 idx++
			printf "water_mark: %02x%02x%02x%02x\n" $water_mark_4 $water_mark_3 $water_mark_2 $water_mark_1
			continue
		fi
		
		if [ "province_code" == "$seg_name" ];then
			let "province_code=seg_value"	
			let bin_array[idx]=province_code idx++		
             printf "province_code: %02x %02x\n" $province_code $dst_province
                      
             if [ "$m_type" == "2" ];then
                if [ $province_code == $dst_province ];then
                    let find_province=1 #找到�?标省份，继续�?
                else
                    break;      #不是�?标省份，不继�?�?
                fi
             fi
                 
			continue
		fi

#sta内部版本
		if [ "sta_3750_version" == "$seg_name" ];then
			in_ver_array[sta_3750_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_3750A_version" == "$seg_name" ];then
			in_ver_array[sta_3750A_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_riscv_version" == "$seg_name" ];then
			in_ver_array[sta_riscv_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_venus_version" == "$seg_name" ];then
			in_ver_array[sta_venus_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_233l_version" == "$seg_name" ];then
			in_ver_array[sta_233l_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi		
		
#cjq2内部版本
		if [ "cjq2_3750_version" == "$seg_name" ];then
			in_ver_array[cjq2_3750_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi	
		
		if [ "cjq2_3750A_version" == "$seg_name" ];then
			in_ver_array[cjq2_3750A_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_venus_version" == "$seg_name" ];then
			in_ver_array[cjq2_venus_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_riscv_version" == "$seg_name" ];then
			in_ver_array[cjq2_riscv_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_233l_version" == "$seg_name" ];then
			in_ver_array[cjq2_233l_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
#cco内部版本
		if [ "cco_3951_version" == "$seg_name" ];then
			in_ver_array[cco_3951_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_3951A_version" == "$seg_name" ];then
			in_ver_array[cco_3951A_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_venus_version" == "$seg_name" ];then
			in_ver_array[cco_venus_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_233l_version" == "$seg_name" ];then
			in_ver_array[cco_233l_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_riscv_version" == "$seg_name" ];then
			in_ver_array[cco_riscv_version]=$seg_value
			#printf "$seg_name: %04x\n" ${in_ver_array[$seg_name]}
			continue
		fi

#sta外部版本
		if [ "sta_3750_out_version" == "$seg_name" ];then
			out_ver_array[sta_3750_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_3750A_out_version" == "$seg_name" ];then
			out_ver_array[sta_3750A_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_riscv_out_version" == "$seg_name" ];then
			out_ver_array[sta_riscv_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_venus_out_version" == "$seg_name" ];then
			out_ver_array[sta_venus_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "sta_233l_out_version" == "$seg_name" ];then
			out_ver_array[sta_233l_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi	

#cjq2外部版本
		if [ "cjq2_3750_out_version" == "$seg_name" ];then
			out_ver_array[cjq2_3750_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi	
		
		if [ "cjq2_3750A_out_version" == "$seg_name" ];then
			out_ver_array[cjq2_3750A_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_venus_out_version" == "$seg_name" ];then
			out_ver_array[cjq2_venus_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_riscv_out_version" == "$seg_name" ];then
			out_ver_array[cjq2_riscv_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cjq2_233l_out_version" == "$seg_name" ];then
			out_ver_array[cjq2_233l_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
#cco外部版本
		if [ "cco_3951_out_version" == "$seg_name" ];then
			out_ver_array[cco_3951_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_3951A_out_version" == "$seg_name" ];then
			out_ver_array[cco_3951A_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_venus_out_version" == "$seg_name" ];then
			out_ver_array[cco_venus_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_233l_out_version" == "$seg_name" ];then
			out_ver_array[cco_233l_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		
		if [ "cco_riscv_out_version" == "$seg_name" ];then
			out_ver_array[cco_riscv_out_version]=$seg_value
			#printf "$seg_name: %04x\n" ${out_ver_array[$seg_name]}
			continue
		fi
		

		
		let idx+=10
#应用功能开�?
		if [ "function_switch_usemode" == "$seg_name" ];then
			let "func_switch+=$seg_value"
			continue
		fi
		
		if [ "DebugeMode" == "$seg_name" ];then
			let "func_switch+=($seg_value << 1)"
			continue
		fi
		
		if [ "NoiseDetectSWC" == "$seg_name" ];then
			let "func_switch += ($seg_value << 2)"
			continue
		fi
		
		if [ "WhitelistSWC" == "$seg_name" ];then
			let "func_switch += ($seg_value << 3)"
			continue
		fi
		
		if [ "UpgradeMode" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 4)"
			continue
		fi
		
		if [ "AODVSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 5)"
			continue
		fi
		
		if [ "EventReportSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 6)"
			continue
		fi
		
		if [ "ModuleIDGetSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 7)"
			continue
		fi
		
		if [ "PhaseSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 8)"
			continue
		fi
		
		if [ "IndentifySWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 9)"
			continue
		fi
		
		if [ "DataAddrFilterSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 10)"
			continue
		fi
		
		if [ "NetSenseSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 11)"
			continue
		fi
		
		if [ "SignalQualitySWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 12)"
			continue
		fi
		
		if [ "SetBandSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 13)"
			continue
		fi
		
		if [ "SwPhase" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 14)"
			continue
		fi
		
		if [ "oop20EvetSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 15)"
			continue
		fi
		
		if [ "oop20BaudSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 16)"
			continue
		fi
		
		if [ "JZQBaudSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 17)"
			continue
		fi
		
		if [ "MinCollectSWC" ==  "$seg_name" ];then
			if [ $province_code = "0x10" ];then
				echo "true"
				#MinCollectSWC="0x01"
				let "func_switch += (0x00000001 << 18)"
			else
				echo "false"
				#MinCollectSWC=$MinCollectSWC
				let "func_switch += ($seg_value << 18)"
			fi
			continue
		fi
		
		if [ "IDInfoGetModeSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 19)"
			printf "func_switch: %08x\n" $func_switch
			continue
		fi

		if [ "TransModeSWC" ==  "$seg_name" ];then
			let "func_switch += ($seg_value << 20)"
			printf "func_switch: %08x\n" $func_switch
			continue
		fi
		
		if [ "function_switch_used" == "$seg_name" ];then
			let "func_switch_used = $seg_value"
			
			let "func_switch_cs = (func_switch & 16#ff) + (func_switch >> 8 & 16#ff) + (func_switch >> 16 & 16#ff) + (func_switch >> 24 & 16#ff) +
									(func_switch_res1 & 16#ff) + (func_switch_res1 >> 8 & 16#ff) + (fun_swtich_res1 >> 16 & 16#ff) + (fun_swtich_res1 >> 24 & 16#ff) +
									(func_switch_res2 & 16#ff) + (func_switch_res2 >> 8 & 16#ff) + func_switch_used"
			let "func_switch_cs &= 16#ff"
			
			
			
			let "v_1=func_switch & 16#ff"
			let "v_2=func_switch >> 8 & 16#ff"
			let "v_3=func_switch >> 16 & 16#ff"
			let "v_4=func_switch >> 24 & 16#ff"
			let bin_array[idx]=v_1 idx++
			let bin_array[idx]=v_2 idx++
			let bin_array[idx]=v_3 idx++
			let bin_array[idx]=v_4 idx++
			
			let "v_1=func_switch_res1 & 16#ff"
			let "v_2=func_switch_res1 >> 8 & 16#ff"
			let "v_3=func_switch_res1 >> 16 & 16#ff"
			let "v_4=func_switch_res1 >> 24 & 16#ff"
			let bin_array[idx]=v_1 idx++
			let bin_array[idx]=v_2 idx++
			let bin_array[idx]=v_3 idx++
			let bin_array[idx]=v_4 idx++
			
			let "v_1=func_switch_res2 & 16#ff"
			let "v_2=func_switch_res2 >> 8 & 16#ff"
			let bin_array[idx]=v_1 idx++
			let bin_array[idx]=v_2 idx++
			
			let bin_array[idx]=func_switch_used  idx++
			let bin_array[idx]=func_switch_cs   idx++
	
			printf "func_switch cs: %02x\n" $func_switch_cs
			continue
		fi
		
#参数配置
		if [ "param_cfg_usemode" == "$seg_name" ];then
			let "param_cfg_usemode = $seg_value"
			continue
		fi
		
		if [ "ConcurrencySize" == "$seg_name" ];then
			let "ConcurrencySize = $seg_value"
			continue
		fi
		
		if [ "ReTranmitMaxNum" == "$seg_name" ];then
			let "ReTranmitMaxNum = $seg_value"
			continue
		fi
		
		if [ "ReTranmitMaxTime" == "$seg_name" ];then
			let "ReTranmitMaxTime = $seg_value"
			continue
		fi
		
		if [ "BroadcastConRMNum" == "$seg_name" ];then
			let "BroadcastConRMNum = $seg_value"
			continue
		fi
		
		if [ "AllBroadcastType" == "$seg_name" ];then
			let "AllBroadcastType = $seg_value"
			continue
		fi
		
		if [ "JZQ_baud_cfg" == "$seg_name" ];then
			let "JZQ_baud_cfg = $seg_value"
			continue
		fi
		
		if [ "param_cfg_used" == "$seg_name" ];then
			let "param_cfg_used = $seg_value"
			
			let "param_cfg_cs = param_cfg_usemode + ConcurrencySize + ReTranmitMaxNum + ReTranmitMaxTime + BroadcastConRMNum + AllBroadcastType + JZQ_baud_cfg + 
								param_cfg_res_array[0] + param_cfg_res_array[1] + param_cfg_res_array[2] + param_cfg_res_array[3] + param_cfg_res_array[4] + param_cfg_res_array[5] + param_cfg_res_array[6] + param_cfg_res_array[7] + param_cfg_res_array[8] + param_cfg_res_array[9] + param_cfg_res_array[10] + param_cfg_used"
			let "param_cfg_cs &= 16#ff"
			
			
			let bin_array[idx]=param_cfg_usemode  idx++
			let bin_array[idx]=ConcurrencySize  idx++
			let bin_array[idx]=ReTranmitMaxNum  idx++
			let bin_array[idx]=ReTranmitMaxTime  idx++
			let bin_array[idx]=BroadcastConRMNum  idx++
			let bin_array[idx]=AllBroadcastType  idx++
			let bin_array[idx]=JZQ_baud_cfg  idx++
			let bin_array[idx]=param_cfg_res_array[0]  idx++
			let bin_array[idx]=param_cfg_res_array[1]  idx++
			let bin_array[idx]=param_cfg_res_array[2]  idx++
			let bin_array[idx]=param_cfg_res_array[3]  idx++
			let bin_array[idx]=param_cfg_res_array[4]  idx++
			let bin_array[idx]=param_cfg_res_array[5]  idx++
			let bin_array[idx]=param_cfg_res_array[6]  idx++
			let bin_array[idx]=param_cfg_res_array[7]  idx++
			let bin_array[idx]=param_cfg_res_array[8]  idx++
			let bin_array[idx]=param_cfg_res_array[9]  idx++
			let bin_array[idx]=param_cfg_res_array[10]  idx++
			
			let bin_array[idx]=param_cfg_used  idx++
			let bin_array[idx]=param_cfg_cs   idx++
			
			printf "param_cfg_cs: %02x\n" $param_cfg_cs
			continue
		fi
		
		:<<!
		for i in $line
		do
			echo $i
		done
		
		for key in ${!module_in_ver_name_array[*]}
			do
				echo "$key -> ${module_in_ver_name_array[$key]}"
			done
!
	done < $config_file_name   #while read -r line
	
    if [ "$m_type" == "2" ];then
        if [ $find_province == 0 ];then
            continue;   #找不到目标省份，继续读下一�?
        fi
    fi
	
	#echo "len: " ${#in_ver_array[@]}
	#echo ${in_ver_array[@]}
	
	if [ "$m_type" == "1" ];then
		let "m_idx=1"
	else
		let "m_idx=5"
	fi
			
	for((m=0; m<$m_idx; m++))
	do
		if [ "$m_type" == "2" ];then
			if [ $m == 0 ];then
				choose_type="t_3750"
			elif [ $m == 1 ];then
				choose_type="t_3750A"
			elif [ $m == 2 ];then
				choose_type="t_venus"
			elif [ $m == 3 ];then
				choose_type="t_233l"
			elif [ $m == 4 ];then
				choose_type="t_riscv"
			else
				echo "wrong m"
				exit
			fi
		fi
		
		for((j=0; j<3; j++))
		do
			v_idx=9
			bin_str=""
			#512字节以使�?51字节，剩�?461字节�?0保留
			dd if=/dev/zero of=null.bin bs=461 count=1
			
			#内部版本		
			if [ "$choose_type" == "t_3750" ];then 
				if [ 0 == $j ];then
					let "iv_h=${in_ver_array[sta_3750_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[sta_3750_version]} & 16#ff"	
					file_name2="sta_3750"
				elif [ 1 == $j ];then
					let "iv_h=${in_ver_array[cjq2_3750_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cjq2_3750_version]} & 16#ff"	
					file_name2="cjq2_3750"
				elif [ 2 == $j ];then
					let "iv_h=${in_ver_array[cco_3951_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cco_3951_version]} & 16#ff"
					file_name2="cco_3951"				
				fi	
			elif [ "$choose_type" == "t_3750A" ];then
				if [ 0 == $j ];then
					let "iv_h=${in_ver_array[sta_3750A_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[sta_3750A_version]} & 16#ff"	
					file_name2="sta_3750A"
				elif [ 1 == $j ];then
					let "iv_h=${in_ver_array[cjq2_3750A_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cjq2_3750A_version]} & 16#ff"	
					file_name2="cjq2_3750A"
				elif [ 2 == $j ];then
					let "iv_h=${in_ver_array[cco_3951A_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cco_3951A_version]} & 16#ff"	
					file_name2="cco_3951A"
				fi	

			#elif [ "$d_type" == "r" ];then
			#	echo "make roland"

			elif [ "$choose_type" == "t_venus" ];then
				if [ 0 == $j ];then
					let "iv_h=${in_ver_array[sta_venus_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[sta_venus_version]} & 16#ff"	
					file_name2="sta_venus"
				elif [ 1 == $j ];then
					let "iv_h=${in_ver_array[cjq2_venus_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cjq2_venus_version]} & 16#ff"	
					file_name2="cjq2_venus"
				elif [ 2 == $j ];then
					let "iv_h=${in_ver_array[cco_venus_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cco_venus_version]} & 16#ff"
					file_name2="cco_venus"				
				fi	
			elif [ "$choose_type" == "t_233l" ];then
				if [ 0 == $j ];then
					let "iv_h=${in_ver_array[sta_233l_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[sta_233l_version]} & 16#ff"	
					file_name2="sta_233l"
				elif [ 1 == $j ];then
					let "iv_h=${in_ver_array[cjq2_233l_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cjq2_233l_version]} & 16#ff"	
					file_name2="cjq2_233l"
				elif [ 2 == $j ];then
					let "iv_h=${in_ver_array[cco_233l_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cco_233l_version]} & 16#ff"	
					file_name2="cco_233l"
				fi	
			elif [ "$choose_type" == "t_riscv" ];then
				if [ 0 == $j ];then
					let "iv_h=${in_ver_array[sta_riscv_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[sta_riscv_version]} & 16#ff"	
					file_name2="sta_riscv"
				elif [ 1 == $j ];then
					let "iv_h=${in_ver_array[cjq2_riscv_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cjq2_riscv_version]} & 16#ff"	
					file_name2="cjq2_riscv"
				elif [ 2 == $j ];then
					let "iv_h=${in_ver_array[cco_riscv_version]} >> 8 & 16#ff"
					let "iv_l=${in_ver_array[cco_riscv_version]} & 16#ff"	
					file_name2="cco_riscv"
				fi	
			else
				echo "unkonw choose_type"
				exit
			fi
			

			
			let bin_array[v_idx]=iv_l v_idx++
			let bin_array[v_idx]=iv_h v_idx++
			printf "in_ver: %02x %02x\n" $iv_h $iv_l		
			
				
		#内部版本日期		
			date_day=$standard_day
			date_month=$standard_month
			date_year=$standard_year
			let bin_array[v_idx]=date_day     v_idx++
			let bin_array[v_idx]=date_month	  v_idx++
			let bin_array[v_idx]=date_year    v_idx++
			#echo "date:" $date_year $date_month $date_day

		#外部版本
			if [ "$choose_type" == "t_3750" ];then 
				if [ 0 == $j ];then
					let "iv_h=${out_ver_array[sta_3750_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[sta_3750_out_version]} & 16#ff"		
				elif [ 1 == $j ];then
					let "iv_h=${out_ver_array[cjq2_3750_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cjq2_3750_out_version]} & 16#ff"	
				elif [ 2 == $j ];then
					let "iv_h=${out_ver_array[cco_3951_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cco_3951_out_version]} & 16#ff"	
				fi
			elif [ "$choose_type" == "t_3750A" ];then
				if [ 0 == $j ];then
					let "iv_h=${out_ver_array[sta_3750A_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[sta_3750A_out_version]} & 16#ff"	
				elif [ 1 == $j ];then
					let "iv_h=${out_ver_array[cjq2_3750A_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cjq2_3750A_out_version]} & 16#ff"	
				elif [ 2 == $j ];then
					let "iv_h=${out_ver_array[cco_3951A_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cco_3951A_out_version]} & 16#ff"	
				fi	
			#elif [ "$d_type" == "r" ];then
			#	echo "make roland"

			elif [ "$choose_type" == "t_venus" ];then
				if [ 0 == $j ];then
					let "iv_h=${out_ver_array[sta_venus_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[sta_venus_out_version]} & 16#ff"	
				elif [ 1 == $j ];then
					let "iv_h=${out_ver_array[cjq2_venus_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cjq2_venus_out_version]} & 16#ff"	
				elif [ 2 == $j ];then
					let "iv_h=${out_ver_array[cco_venus_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cco_venus_out_version]} & 16#ff"	
				fi
			elif [ "$choose_type" == "t_233l" ];then
				if [ 0 == $j ];then
					let "iv_h=${out_ver_array[sta_233l_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[sta_233l_out_version]} & 16#ff"	
				elif [ 1 == $j ];then
					let "iv_h=${out_ver_array[cjq2_233l_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cjq2_233l_out_version]} & 16#ff"	
				elif [ 2 == $j ];then
					let "iv_h=${out_ver_array[cco_233l_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cco_233l_out_version]} & 16#ff"	
				fi	

			elif [ "$choose_type" == "t_riscv" ];then
				if [ 0 == $j ];then
					let "iv_h=${out_ver_array[sta_riscv_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[sta_riscv_out_version]} & 16#ff"	
				elif [ 1 == $j ];then
					let "iv_h=${out_ver_array[cjq2_riscv_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cjq2_riscv_out_version]} & 16#ff"	
				elif [ 2 == $j ];then
					let "iv_h=${out_ver_array[cco_riscv_out_version]} >> 8 & 16#ff"
					let "iv_l=${out_ver_array[cco_riscv_out_version]} & 16#ff"	
				fi
			else
				echo "unkonw choose_type"
				exit
			fi
			
			let bin_array[v_idx]=iv_l v_idx++
			let bin_array[v_idx]=iv_h v_idx++
			printf "out_ver: %02x %02x\n" $iv_h $iv_l		
			

			#外部版本日期
			let bin_array[v_idx]=date_day     v_idx++
			let bin_array[v_idx]=date_month	  v_idx++
			let bin_array[v_idx]=date_year    v_idx++
			#echo "out_date:" $date_year $date_month $date_day
			
			get_crc16_value ${bin_array[*]}
			let "iv_h=$CRC16Value >> 8 & 16#FF"
			let "iv_l=$CRC16Value & 16#ff"
			let bin_array[0]=iv_l
			let bin_array[1]=iv_h
			
			for key in ${!bin_array[*]}
			do
				printf "%02x " ${bin_array[$key]}
				bin_str+=$(awk 'BEGIN{printf("%02x",'${bin_array[$key]}')}')
			done
			printf "\n"
			
			file_name1="$province_name"$(awk 'BEGIN{printf("_%02x_config_",'$province_code')}')"$file_name2"
			file_name=$file_name1".bin"  #$fn_year$fn_month$fn_day$fn_hour$fn_min$fn_sec".bin"
			echo "$file_name"
			
			echo $bin_str
			echo $bin_str | xxd -r -ps  > tmp_bin.bin
			cat tmp_bin.bin null.bin > $config_Path$file_name
			rm null.bin tmp_bin.bin
			
			unset bin_array[0]
			unset bin_array[1]
			
		done #for((j=0; j<3; j++))
	
	done #for((m=0; m<$m_idx; m++))
    
    if [ "$m_type" == "2" ];then
        if [ $find_province == 1 ];then
            break;  #找到�?标省份且生成完成，结�?
        fi
    fi

done #for((ii=0; ii < (total_col - 1);ii++))