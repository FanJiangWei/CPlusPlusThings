#!/bin/bash
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8

make clean

read -p $'请选择您所属的电力公司：\e[1;32m回车默认国网，输入0是南网\e[0m：' std

if [ -z "$std" ]; then
    std="gw"
    sed -i "s/STD_GD/STD_2016/g"  `grep STD_GD -rl ./Makefile`    
    echo "set $std 2016"
elif [ "$std" == "0" ];then
    std="gd"
    sed -i "s/STD_2016/STD_GD/g"  `grep STD_2016 -rl ./Makefile`
    echo "set $std GD"
else
    echo "$std 是什么？"    
    exit
fi

#是否需要包含ext_info
read -p $'包含512字节扩展？\e[1;32m回车默认包含，输入其他不包含\e[0m：' ext_info_flag

if [ -z "$ext_info_flag" ]; then
  ext_info_flag=$((0x01))
else
  ext_info_flag=$((0x00))
fi

# 使用字符串方式接收用户输入
read -p $'请选择组合选项：0(国网), 1~34(省份), 99(全部) , \e[1;32m直接回车默认为0（国网）\e[0m：' input_option

# 如果用户没有输入，则默认为 0
if [ -z "$input_option" ]; then
    echo "未选择选项，默认为 0(国网)..."
    combine_option=00
else
    # 将输入转换为十进制整数并添加前导零
    combine_option=$(printf "%02d" "$((10#$input_option))")
    # 将处理后的值重新赋值给 input_option
    input_option=$combine_option
fi

# 后续判断逻辑不变
if [ $combine_option -ne 99 ] && [ $combine_option -lt 0 -o $combine_option -gt 34 ]; then
    echo "未知的组合选项！"
    exit
fi

declare -A province_options=(
    ["00"]="GW"
    ["01"]="BEI_JING"
    ["02"]="TIAN_JIN"
    ["03"]="SHANG_HAI"
    ["04"]="CHONG_QING"
    ["05"]="HE_BEI"
    ["06"]="SHAN_XI"
    ["07"]="LIAO_NING"
    ["08"]="JI_LIN"
    ["09"]="HEI_LONG_JIANG"
    ["10"]="JIANG_SU"
    ["11"]="ZHE_JIANG"
    ["12"]="AN_HUI"
    ["13"]="FU_JIAN"
    ["14"]="JIANG_XI"
    ["15"]="SHAN_DONG"
    ["16"]="HE_NAN"
    ["17"]="HU_BEI"
    ["18"]="HU_NAN"
    ["19"]="GUANG_DONG"
    ["20"]="HAI_NAN"
    ["21"]="SI_CHUAN"
    ["22"]="GUI_ZHOU"
    ["23"]="YUN_NAN"
    ["24"]="SHAAN_XI"
    ["25"]="GAN_SU"
    ["26"]="QING_HAI"
    ["27"]="TAI_WAN"
    ["28"]="NEI_MENG_GU"
    ["29"]="GUANG_XI"
    ["30"]="XI_ZANG"
    ["31"]="NING_XIA"
    ["32"]="XIN_JIANG"
    ["33"]="JI_BEI"
    ["34"]="AO_MEN"
)

province="${province_options[$combine_option]}"

echo $'\e[1;32m添加宏定义 : '$province$'\e[0m'

read -p $'请选择台体类型：\e[1;32m回车默认为双模台体，输入0选择单载波台体\e[0m：' HPLC_HRF_FG

if [ -z "$HPLC_HRF_FG" ]; then
    HPLC_HRF_FG="hrf"
    echo "$province 双模台体走起！"
elif [ "$HPLC_HRF_FG" == "0" ]; then
    HPLC_HRF_FG="hplc"
    echo "$province 单载波台体走起！"
else
    echo "$HPLC_HRF_FG 是什么东西？"
    exit
fi

if [ "$HPLC_HRF_FG" == "hplc" ] || [ "$HPLC_HRF_FG" == "HPLC" ]; then
    mv ../src/datalink/dl_mgm_msg.h ../src/datalink/dl_mgm_msgbk.h
    sed '/HPLC_HRF_PLATFORM_TEST/c\//#define HPLC_HRF_PLATFORM_TEST' ../src/datalink/dl_mgm_msgbk.h > ../src/datalink/dl_mgm_msg.h
    rm ../src/datalink/dl_mgm_msgbk.h
else
    mv ../src/datalink/dl_mgm_msg.h ../src/datalink/dl_mgm_msgbk.h
    sed '/HPLC_HRF_PLATFORM_TEST/c\#define HPLC_HRF_PLATFORM_TEST' ../src/datalink/dl_mgm_msgbk.h > ../src/datalink/dl_mgm_msg.h
    rm ../src/datalink/dl_mgm_msgbk.h
fi


# 函数：设置 image_c 对应的值
set_image_c() {
    case "$1" in
        "") image_c="2" ;;
        "0") image_c="v" ;;
        "1") image_c="d" ;;
        "2") image_c="r" ;;
        "3") image_c="u" ;;
        *) echo "$1 是什么东西？" ; exit ;;
    esac
}

# 提示用户选择芯片类型，并设置 image_c 的值
read -p $'请选择芯片类型：\e[1;32m回车默认为 233L\e[0m（输入 0 选择 RISC-V，输入 1 选择 Venus，输入 2 选择 R (1M)，输入 3 选择 U (2M)）: ' image_c

set_image_c "$image_c"

# 开始编译原始BIN文件
python3 -B make.py "$image_c" "$province"

#生成正式BIN文件并添加header
python3 -B ./get_define_value.py "$combine_option" "$image_c" "$ext_info_flag"

function add_config_bin() {
    # 绕开加密软件，shell可以直接读取，但是PYTHON读取报错
    cat province_config_table.csv > config_data

    # 获取combine_option和image_c的值
    combine_option_hex=$(printf "0x%02d" $combine_option)
	# 添加512字节配置信息
    python3 -B ./make_config_bin.py $combine_option_hex $image_c
}

if [[ $ext_info_flag -eq 1 ]]; then
	# 计算文件大小补齐
    python3 -B ./get_file_sizes.py

	add_config_bin

fi

# 制作D版程序
python3 -B ./make_D_bin.py $image_c

exit