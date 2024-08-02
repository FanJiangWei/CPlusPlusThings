#!/bin/bash

make clean

MYDATE_Y=`date +%-y`
MYDATE_M=`date +%-m`
MYDATE_D=`date +%-d`

mv ../src/include/Version.h ../src/include/Versionbk.h
sed '/#define InnerDate_Y/c\#define InnerDate_Y '$MYDATE_Y'' ../src/include/Versionbk.h > ../src/include/Version.h
rm ../src/include/Versionbk.h

mv ../src/include/Version.h ../src/include/Versionbk.h
sed '/#define InnerDate_M/c\#define InnerDate_M '$MYDATE_M'' ../src/include/Versionbk.h > ../src/include/Version.h
rm ../src/include/Versionbk.h

mv ../src/include/Version.h ../src/include/Versionbk.h
sed '/#define InnerDate_D/c\#define InnerDate_D '$MYDATE_D'' ../src/include/Versionbk.h > ../src/include/Version.h
rm ../src/include/Versionbk.h

MYDATE_Y=`date +%y`
MYDATE_M=`date +%m`
MYDATE_D=`date +%d`
MYDATE_H=`date +%H`
MYDATE_MIN=`date +%M`

read -p "Please input your STD:" std

declare -i std

if [ "$std" == "" ];then
    echo "You don't input your STD...."
    exit
fi

if [ "$std" == "gw" ];then
	
	sed -i "s/STD_GD/STD_2016/g"  `grep STD_GD -rl ./Makefile`
	
	echo "set 2016"
elif [ "$std" == "gd" ];then

	sed -i "s/STD_2016/STD_GD/g"  `grep STD_2016 -rl ./Makefile`
	echo "set GD"
else
	echo "unkonw std"	
	exit
fi


JNUM=`grep -c ^processor /proc/cpuinfo 2>/dev/null`
echo $JNUM
JUSE=`expr $JNUM `
echo $JUSE

#选择需要编译的image，u-> unicorn   r->roland  z->zc3 d->3780 v->3780U
read -p "Please input your Device,u(2M) r(1M) d(3780): v(3780U)" image_c

declare -i image_c

if [ "$image_c" == "" ];then
    echo "You don't input your image_c...."
    exit
fi

if [ "$image_c" == "z" ];then
	echo "make zc"
	make -j$JUSE ZC3750STA
	make -j$JUSE ZC3750CJQ2
	make -j$JUSE ZC3951CCO
elif [ "$image_c" == "u" ];then
	echo "make unicorn"
	sed -i "s/CHIP_R/CHIP_U/g"  `grep CHIP_R -rl ./Makefile`
	sed -i "s/CHIP_D/CHIP_U/g"  `grep CHIP_D -rl ./Makefile`
	sed -i "s/CHIP_V/CHIP_U/g"  `grep CHIP_V -rl ./Makefile`
	echo "set CHIP_U"
	make -j$JUSE unicorn2m_sta
	make -j$JUSE unicorn2m_clt2
	make -j$JUSE unicorn8m_cco
elif [ "$image_c" == "r" ];then
	echo "make roland"
	sed -i "s/CHIP_U/CHIP_R/g"  `grep CHIP_U -rl ./Makefile`
	sed -i "s/CHIP_D/CHIP_R/g"  `grep CHIP_D -rl ./Makefile`
	sed -i "s/CHIP_V/CHIP_R/g"  `grep CHIP_V -rl ./Makefile`
	echo "set CHIP_R"
	make -j$JUSE roland1_1m_sta
	make -j$JUSE roland1_1m_clt2
	make -j$JUSE roland9_1m_cco
elif [ "$image_c" == "d" ];then
	echo "make venus"
	sed -i "s/CHIP_U/CHIP_D/g"  `grep CHIP_U -rl ./Makefile`
	sed -i "s/CHIP_R/CHIP_D/g"  `grep CHIP_R -rl ./Makefile`
	sed -i "s/CHIP_V/CHIP_D/g"  `grep CHIP_V -rl ./Makefile`
	echo "set CHIP_D"
	make -j$JUSE venus2m_sta
	make -j$JUSE venus2m_clt2
	make -j$JUSE venus8m_cco
	make -j$JUSE venus2m_riscv_sta
	make -j$JUSE venus2m_233l_sta
	make -j$JUSE venus8m_233l_cco
elif [ "$image_c" == "v" ];then
	echo "make venus"
	sed -i "s/CHIP_U/CHIP_V/g"  `grep CHIP_U -rl ./Makefile`
	sed -i "s/CHIP_R/CHIP_V/g"  `grep CHIP_R -rl ./Makefile`
	sed -i "s/CHIP_D/CHIP_V/g"  `grep CHIP_D -rl ./Makefile`
	echo "set CHIP_V"
	make -j venus8m_v3_233l_cco
	make -j venus2m_v3_233l_sta
	make -j venus2m_v3_233l_clt2
else
	echo "unkonw image_c"
	exit
fi

V="V"

MYDATE=`date +%y%m%d`

vendor=`grep "vendor\[VENDORSIZE]" ../src/datalink/dl_mgm_msg.c | awk '{print $4}'`

echo $vendor 

vendor=${vendor%\"*}

vendor=${vendor#*\"}

echo "vendor                : $vendor"

function BuildVersionNum {
	echo "BuildVersionNum"
	
	#Property
	s_Property=`grep -n PROPERTY ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_Property
	Property=${s_Property:0:1}
	echo $Property
	
	
	s_sta_x1=`grep -n ZC3750STA_Innerver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_sta_x1
	staver1=${s_sta_x1:2:2}
	echo $staver1
	staver1_riscv=0${s_sta_x1:3:1}
	echo $staver1_riscv
	
	s_sta_x2=`grep -n ZC3750STA_Innerver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_sta_x2
	#printf "%d\n" $staver22
	staver2=0${s_sta_x2:3:1}
	echo $staver2
	staver2_233l=${s_sta_x2:2:2}
	echo $staver2_233l
	#get sta  outerversion 
	s_staout_x1=`grep -n ZC3750STA_ver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_staout_x1
	staoutver1=${s_staout_x1:2:2}
	echo $staoutver1
	s_staout_x2=`grep -n ZC3750STA_ver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_staout_x2
	staoutver2=${s_staout_x2:2:2}
	echo $staoutver2
	#get sta  chip
	s_sta_chip1=`grep -n ZC3750STA_chip1 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_sta_chip1
	stachip1=${s_sta_chip1:0:1}
	echo $stachip1
	s_sta_chip2=`grep -n ZC3750STA_chip2 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_sta_chip2
	stachip2=${s_sta_chip2:0:1}
	echo $stachip2
	#
	s_product_func=`grep -n PRODUCT_func ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_product_func
	productfunc=${s_product_func:0:1}
	echo $productfunc
	#
	s_chip_code=`grep -n CHIP_code ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_chip_code
	chipcode=${s_chip_code:0:1}
	echo $chipcode
	#
	s_area_code=`grep -n AREA_code ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_area_code
	areacode=${s_area_code:2:2}
	echo $areacode
	#
	s_sta_type=`grep -n ZC3750STA_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_sta_type
	statype=${s_sta_type:0:1}
	echo $statype
	#
	s_sta_up_type=`grep -n ZC3750STA_UpDate_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_sta_up_type
	stauptype=${s_sta_up_type:0:1}
	echo $stauptype
	#
	s_power_off=`grep -n POWER_OFF ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_power_off
	poweroff=${s_power_off:0:1}
	echo $poweroff
	#get sta  pr 
	s_sta_prtcl=`grep -n ZC3750STA_prtcl ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_sta_prtcl
	staprtcl=${s_sta_prtcl:2:2}
	echo $staprtcl
	s_sta_prdct=`grep -n ZC3750STA_prdct ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_sta_prdct
	staprdct=${s_sta_prdct:2:2}
	echo $staprdct
	ver_sta=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1$staver2$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta
	ver_sta_c=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1$staver2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta_c
	ver_sta_riscv=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1_riscv$staver2\$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta_riscv
	ver_sta_riscv_c=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1_riscv$staver2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta_riscv_c
	ver_sta_233l=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1$staver2_233l\$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta_233l
	ver_sta_233l_c=`echo ZC$productfunc$chipcode$statype-$staprtcl$poweroff$staprdct-R$staver1$staver2_233l\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_sta_233l_c
	#ver_sta=`echo ZC$stachip1$stachip2$MYDATE\V$staver1$staver2`
	#echo $ver_sta

	#get cjq2  innerversion 
	s_cjq2_x1=`grep -n ZC3750CJQ2_Innerver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cjq2_x1
	cjq2ver1=${s_cjq2_x1:2:2}
	echo $cjq2ver1
	s_cjq2_x2=`grep -n ZC3750CJQ2_Innerver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cjq2_x2
	cjq2ver2=${s_cjq2_x2:2:2}
	echo $cjq2ver2
	#get cjq2  outerversion 
	s_cjq2out_x1=`grep -n ZC3750CJQ2_ver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cjq2out_x1
	cjq2outver1=${s_cjq2out_x1:2:2}
	echo $cjq2outver1
	s_cjq2out_x2=`grep -n ZC3750CJQ2_ver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cjq2out_x2
	cjq2outver2=${s_cjq2out_x2:2:2}
	echo $cjq2outver2
	#get cjq2  chip
	s_cjq2_chip1=`grep -n ZC3750CJQ2_chip1 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cjq2_chip1
	cjq2chip1=${s_cjq2_chip1:0:1}
	echo $cjq2chip1
	s_cjq2_chip2=`grep -n ZC3750CJQ2_chip2 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cjq2_chip2
	cjq2chip2=${s_cjq2_chip2:0:1}
	echo $cjq2chip2
	#
	cjq2_type=`grep -n ZC3750CJQ2_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $cjq2_type
	cjq2type=${cjq2_type:0:1}
	echo $cjq2type
	#
	s_cjq2_up_type=`grep -n ZC3750CJQ2_UpDate_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cjq2_up_type
	cjq2uptype=${s_cjq2_up_type:0:1}
	echo $cjq2uptype
	#get cjq  pr 
	s_cjq2_prtcl=`grep -n ZC3750CJQ2_prtcl ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_cjq2_prtcl
	cjq2prtcl=${s_cjq2_prtcl:2:2}
	echo $cjq2prtcl
	s_cjq2_prdct=`grep -n ZC3750CJQ2_prdct ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_cjq2_prdct
	cjq2prdct=${s_cjq2_prdct:2:2}
	echo $cjq2prdct
	ver_cjq2=`echo ZC$productfunc$chipcode$cjq2type-$cjq2prtcl$poweroff$cjq2prdct-R$cjq2ver1$cjq2ver2$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cjq2
	ver_cjq2_c=`echo ZC$productfunc$chipcode$cjq2type-$cjq2prtcl$poweroff$cjq2prdct-R$cjq2ver1$cjq2ver2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cjq2_c
	ver_cjq2_riscv=`echo ZC$productfunc$chipcode$cjq2type-$cjq2prtcl$poweroff$cjq2prdct-R$cjq2ver1$cjq2ver2$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cjq2_riscv
	ver_cjq2_riscv_c=`echo ZC$productfunc$chipcode$cjq2type-$cjq2prtcl$poweroff$cjq2prdct-R$cjq2ver1$cjq2ver2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cjq2_riscv_c

	#get cco  innerversion 
	s_cco_x1=`grep -n ZC3951CCO_Innerver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cco_x1
	ccover1=${s_cco_x1:2:2}
	echo $ccover1
	s_cco_x2=`grep -n ZC3951CCO_Innerver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_cco_x2
	ccover2=0${s_cco_x2:3:1}
	echo $ccover2
	ccover_233l_2=${s_cco_x2:2:2}
	echo $ccover_233l_2
	#get cco  outerversion 
	s_ccoout_x1=`grep -n ZC3951CCO_ver1 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_ccoout_x1
	ccooutver1=${s_ccoout_x1:2:2}
	echo $ccooutver1
	s_ccoout_x2=`grep -n ZC3951CCO_ver2 ../src/include/Version.h | head -$1 | tail -1 | awk '{print $3}'`
	echo $s_ccoout_x2
	ccooutver2=${s_ccoout_x2:2:2}
	echo $ccooutver2
	#get cco  chip
	s_cco_chip1=`grep -n ZC3951CCO_chip1 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cco_chip1
	ccochip1=${s_cco_chip1:0:1}
	echo $ccochip1
	s_cco_chip2=`grep -n ZC3951CCO_chip2 ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cco_chip2
	ccochip2=${s_cco_chip2:0:1}
	echo $ccochip2
	#
	s_cco_type=`grep -n ZC3951CCO_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cco_type
	ccotype=${s_cco_type:0:1}
	echo $ccotype
	#
	s_cco_up_type=`grep -n ZC3951CCO_UpDate_type ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}' | sed s/\'//g`
	echo $s_cco_up_type
	ccouptype=${s_cco_up_type:0:1}
	echo $ccouptype
	#get cco  pr 
	s_cco_prtcl=`grep -n ZC3951CCO_prtcl ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_cco_prtcl
	ccoprtcl=${s_cco_prtcl:2:2}
	echo $ccoprtcl
	s_ccoprtcl_prdct=`grep -n ZC3951CCO_prdct ../src/include/Version.h | head -$2 | tail -1 | awk '{print $3}'`
	echo $s_ccoprtcl_prdct
	ccoprdct=${s_ccoprtcl_prdct:2:2}
	echo $ccoprdct
	ver_cco=`echo ZC$productfunc$chipcode$ccotype-$ccoprtcl$poweroff$ccoprdct-R$ccover1$ccover2$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cco
	ver_cco_c=`echo ZC$productfunc$chipcode$ccotype-$ccoprtcl$poweroff$ccoprdct-R$ccover1$ccover2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cco_c
	
	ver_cco_233l=`echo ZC$productfunc$chipcode$ccotype-$ccoprtcl$poweroff$ccoprdct-R$ccover1$ccover_233l_2$Property.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cco_233l
	ver_cco_233l_c=`echo ZC$productfunc$chipcode$ccotype-$ccoprtcl$poweroff$ccoprdct-R$ccover1$ccover_233l_2\D.$areacode-$MYDATE_Y$MYDATE_M$MYDATE_D$MYDATE_H$MYDATE_MIN`
	echo $ver_cco_233l_c

	
	#get Date_Y  Date_M Date_D 
	s_date_Y=`grep -n Date_Y ../src/include/Version.h | tr "\r\n" "\n"| awk '{print $3}'`
	echo $s_date_Y
	dateY=${s_date_Y:0:2}
	echo $dateY
	s_date_M=`grep -n Date_M ../src/include/Version.h | tr "\r\n" "\n"| awk '{print $3}'`
	echo $s_date_M
	dateM=${s_date_M:0:2}
	echo $dateM
	s_date_D=`grep -n Date_D ../src/include/Version.h | tr "\r\n" "\n"| awk '{print $3}'`
	echo $s_date_D
	dateD=${s_date_D:0:2}
	echo $dateD
	let "dateymd=($dateY&0x7F)|(($dateM<<7)&0x780)|(($dateD<<11)&0xF800)"
	echo $dateymd
	let "dateymd1=$dateymd&0xFF"
	echo $dateymd1
	let "dateymd2=($dateymd&0xFF00)>>8"
	echo $dateymd2

}
#BuildVersionNum 第一个参数为 DN 内外版本号 第二参数为PJU TB等

if [ "$std" == "gw" ];then
 
	if [ "$image_c" == "u" ];then
		BuildVersionNum 2 1
	elif [ "$image_c" == "r" ];then
		BuildVersionNum	1 1
	elif [ "$image_c" == "d" ];then
		BuildVersionNum	3 3
	elif [ "$image_c" == "v" ];then
		BuildVersionNum	4 3
	fi
elif [ "$std" == "gd" ];then
	if [ "$image_c" == "u" ];then
		BuildVersionNum 6 2
		
	elif [ "$image_c" == "r" ];then
		BuildVersionNum 5 2
	fi
else
	echo "unkonw"
	exit
fi
myPath="../STD_2016bin/" 
	if [ ! -d "$myPath" ]; then  
		mkdir "$myPath"
	fi

#-------------------------------------------------------------------------------------------
if [ "$ver_cjq2.bin" == "$ver_sta.bin" ];then
	echo " $ver_cjq2 eq $ver_sta please change version"
	exit
fi

if [ "$image_c" == "z" ];then
#ZC3750STA
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$stauptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_ZC3750STA.bin > $ver_sta.bin
#ZC3750CJQ2
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_ZC3750CJQ2.bin > $ver_cjq2.bin
#ZC3951CCO
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_ZC3951CCO.bin > $ver_cco.bin

elif [ "$image_c" == "u" ];then
#unicorn_STA
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$stauptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_unicorn2m_sta.bin > $ver_sta.bin
#unicorn_CJQ
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_unicorn2m_clt2.bin > $ver_cjq2.bin
#unicorn_CCO
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_unicorn8m_cco.bin > $ver_cco.bin

elif [ "$image_c" == "r" ];then
#roland_STA    STA use "M"    IOT_C use "H"     PLC_4438 use "D"
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$stauptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_roland1_1m_sta.bin > $ver_sta.bin
#roland_CJQ
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_roland1_1m_clt2.bin > $ver_cjq2.bin
#roland_CCO
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_roland9_1m_cco.bin > $ver_cco.bin

elif [ "$image_c" == "d" ];then
##roland_STA    STA use "M"    IOT_C use "H"     PLC_4438 use "D"
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$stauptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_mizar1m_sta.bin > $ver_sta.bin
##roland_CJQ
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_mizar1m_clt2.bin > $ver_cjq2.bin
##roland_CCO
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x5A\x43\x48\x43\x33\x37\x35\x30\x41\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_mizar9m_cco.bin > $ver_cco.bin
#ZC3750STA
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver2_233l\x$staver2$chipcode$stauptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_venus2m_sta.bin > $ver_sta.bin
#ZC3750CJQ2
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_venus2m_clt2.bin > $ver_cjq2.bin
#ZC3951CCO
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_venus8m_cco.bin > $ver_cco.bin
#ZC3750STA
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver2_233l\x$staver2$chipcode$stauptype\x5A\x43\x48\x43\x33\x37\x38\x30\x30\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_venus2m_riscv_sta.bin > $ver_sta_riscv.bin
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver2_233l\x$staver2$chipcode$stauptype\x5A\x43\x48\x43\x33\x37\x38\x30\x31\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_venus2m_233l_sta.bin > $ver_sta_233l.bin
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover_233l_2$chipcode$ccouptype\x5A\x43\x48\x43\x33\x37\x38\x30\x31\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_venus8m_233l_cco.bin > $ver_cco_233l.bin
#ZC3750CJQ2
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_venus2m_riscv_clt2.bin > $ver_cjq2.bin
#ZC3951CCO
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_venus8m_riscv_cco.bin > $ver_cco.bin
elif [ "$image_c" == "v" ];then
##roland_STA    STA use "M"    IOT_C use "H"     PLC_4438 use "D"

#ZC3750CJQ2
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2uptype\x5A\x43\x48\x43\x33\x37\x38\x30\x33\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_venus2m_v3_233l_clt2.bin > $ver_cjq2.bin
#ZC3951CCO
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccouptype\x5A\x43\x48\x43\x33\x37\x38\x30\x33\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_venus8m_v3_233l_cco.bin > $ver_cco.bin
#ZC3750STA
sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$stauptype\x5A\x43\x48\x43\x33\x37\x38\x30\x33\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_venus2m_v3_233l_sta.bin > $ver_sta.bin
else
	echo "unkonw image_c"
	exit
fi


#ZC3750STA
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$statype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_ZC3750STA.bin > $ver_sta.bin
#unicorn
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$statype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_unicorn2m_sta.bin > $ver_sta.bin
#roland
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$staver1\x$staver2$chipcode$statype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$staoutver2\x$staoutver1\d$dateymd1\d$dateymd2/g" image_roland1_1m_sta.bin > $ver_sta.bin

echo "ZC3750sta_version     : $ver_sta "

#--------------------------------------------------------------------------------
#ZC3750CJQ2
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2type\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_ZC3750CJQ2.bin > $ver_cjq2.bin
#unicorn
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2type\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_unicorn2m_clt2.bin > $ver_cjq2.bin
#roland
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$cjq2ver1\x$cjq2ver2$chipcode$cjq2type\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$cjq2outver2\x$cjq2outver1\d$dateymd1\d$dateymd2/g" image_roland1_1m_clt2.bin > $ver_cjq2.bin
echo "ZC3750cjq2_version    : $ver_cjq2 "

#--------------------------------------------------------------------------------
#ZC3951CCO
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccotype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_ZC3951CCO.bin > $ver_cco.bin
#unicorn
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccotype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_unicorn8m_cco.bin > $ver_cco.bin
#roland
#sed "s/\x07\x02\x05\x09\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x00\x00\x00\x00/\x$ccover1\x$ccover2$chipcode$ccotype\x74\x72\x69\x64\x75\x63\x74\x6F\x72\x00\x00\x00\x$ccooutver2\x$ccooutver1\d$dateymd1\d$dateymd2/g" image_roland9_1m_cco.bin > $ver_cco.bin

echo "ZC3951_version        : $ver_cco "


#dd if=$ver_sta.bin  bs=60000 count=0 of=dest.bin  
#dd if=/dev/sda bs=1K count=600 of=first.bin  
echo "type befor juge $image_c"
if [ "$image_c" == "u" ];then
	echo "combin u type befor "
	if [ -e "../BOOT/ZBHn190820V0210.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBHn190820V0210.bin`
		let bootsize=0x60000-bootsize
		dd if=/dev/zero of=first.bin bs=$bootsize count=1
		cat ../BOOT/ZBHn190820V0210*.bin first.bin >dest.bin
		cat dest.bin $ver_sta.bin >$ver_sta_c.bin
		cat dest.bin $ver_cjq2.bin >$ver_cjq2_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_c.bin success!"
		echo "Combine $ver_cjq2_c.bin success!"
	else
		echo " ZBHn190820V0210.bin is not exists, please add boot ZBHn190820V0210.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/ZBDN190820V0210.bin" ];then
		echo $ccobootsize
		ccobootsize=`wc -c < ../BOOT/ZBDN190820V0210.bin`
		let ccobootsize=0x60000-ccobootsize
		dd if=/dev/zero of=first.bin bs=$ccobootsize count=1
		cat ../BOOT/ZBDN190820V0210*.bin first.bin >dest.bin
		cat dest.bin $ver_cco.bin >$ver_cco_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_cco_c.bin sucess!"
	else
		echo " ZBDN190820V0210.bin is not exists, please add boot ZBDN190820V0210.bin in ../BOOT"
		exit
	fi
fi
if [ "$image_c" == "r" ];then
	echo "combin r type befor "
	if [ -e "../BOOT/ZBHn210902V0350.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBHn210902V0350.bin`
		let bootsize=0x60000-bootsize
		dd if=/dev/zero of=first.bin bs=$bootsize count=1
		cat ../BOOT/ZBHn210902V0350*.bin first.bin >dest.bin
		cat dest.bin $ver_sta.bin >$ver_sta_c.bin
		cat dest.bin $ver_cjq2.bin >$ver_cjq2_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_c.bin success!"
		echo "Combine $ver_cjq2_c.bin success!"
	else
		echo " ZBHn210902V0350.bin is not exists, please add boot ZBHn210902V0350.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/ZBDN210603V0340.bin" ];then
		echo $ccobootsize
		ccobootsize=`wc -c < ../BOOT/ZBDN210603V0340.bin`
		let ccobootsize=0x60000-ccobootsize
		dd if=/dev/zero of=first.bin bs=$ccobootsize count=1
		cat ../BOOT/ZBDN210603V0340*.bin first.bin >dest.bin
		cat dest.bin $ver_cco.bin >$ver_cco_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_cco_c.bin sucess!"
	else
		echo " ZBDN210603V0340.bin is not exists, please add boot ZBDN210603V0340.bin in ../BOOT"
		exit
	fi

fi
if [ "$image_c" == "d" ];then
	echo "combin d type befor "
	if [ -e "../BOOT/boot_venus2m.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/boot_venus2m.bin`
		let bootsize=0x60000-bootsize
		dd if=/dev/zero of=first.bin bs=$bootsize count=1
		cat ../BOOT/boot_venus2m.bin first.bin >dest.bin
		cat dest.bin $ver_sta.bin >$ver_sta_c.bin
		cat dest.bin $ver_cjq2.bin >$ver_cjq2_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_c.bin success!"
		echo "Combine $ver_cjq2_c.bin success!"
	else
		echo " boot_venus2m.bin is not exists, please add boot boot_venus2m.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/boot_venus8m.bin" ];then
		echo $ccobootsize
		ccobootsize=`wc -c < ../BOOT/boot_venus8m.bin`
		let ccobootsize=0x60000-ccobootsize
		dd if=/dev/zero of=first.bin bs=$ccobootsize count=1
		cat ../BOOT/boot_venus8m.bin first.bin >dest.bin
		cat dest.bin $ver_cco.bin >$ver_cco_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_cco_c.bin sucess!"
	else
		echo " boot_venus8m.bin is not exists, please add boot boot_venus8m.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/boot_venus2m_riscv.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/boot_venus2m_riscv.bin`
		let bootsize=0x60000-bootsize
		#dd if=/dev/zero of=first.bin bs=$bootsize count=1
		#let bootsize=bootsize-1
		tr '\000' '\377' < /dev/zero | dd of=first.bin bs=1 count=$bootsize
		cat ../BOOT/boot_venus2m_riscv.bin first.bin >dest.bin
		cat dest.bin $ver_sta_riscv.bin >$ver_sta_riscv_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_riscv_c.bin success!"
	else
		echo " boot_venus2m_riscv.bin is not exists, please add boot boot_venus2m_riscv.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/ZBHo221031V0010.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBHo221031V0010.bin`
		let bootsize=0x60000-bootsize
		#dd if=/dev/zero of=first.bin bs=$bootsize count=1
		#let bootsize=bootsize-1
		tr '\000' '\377' < /dev/zero | dd of=first.bin bs=1 count=$bootsize
		cat ../BOOT/ZBHo221031V0010.bin first.bin >dest.bin
		cat dest.bin $ver_sta_233l.bin >$ver_sta_233l_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_233l_c.bin success!"
	else
		echo " ZBHo221031V0010.bin is not exists, please add boot ZBHo221031V0010.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/ZBDO221031V0010.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBDO221031V0010.bin`
		let bootsize=0x60000-bootsize
		#dd if=/dev/zero of=first.bin bs=$bootsize count=1
		#let bootsize=bootsize-1
		tr '\000' '\377' < /dev/zero | dd of=first.bin bs=1 count=$bootsize
		cat ../BOOT/ZBDO221031V0010.bin first.bin >dest.bin
		cat dest.bin $ver_cco_233l.bin >$ver_cco_233l_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_233l_c.bin success!"
	else
		echo " ZBDO221031V0010.bin is not exists, please add boot ZBDO221031V0010.bin in ../BOOT"
		exit
	fi

fi
if [ "$image_c" == "v" ];then
	echo "combin d type befor "
	if [ -e "../BOOT/ZBHo221226V3010.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBHo221226V3010.bin`
		let bootsize=0x60000-bootsize
		dd if=/dev/zero of=first.bin bs=$bootsize count=1
		cat ../BOOT/ZBHo221226V3010.bin first.bin >dest.bin
		cat dest.bin $ver_sta.bin >$ver_sta_c.bin
		cat dest.bin $ver_cjq2.bin >$ver_cjq2_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_c.bin success!"
		echo "Combine $ver_cjq2_c.bin success!"
	else
		echo " ZBHo221226V3010.bin is not exists, please add boot ZBHo221226V3010.bin in ../BOOT"
		exit
	fi
	if [ -e "../BOOT/ZBDO230112V3010.bin" ];then
		echo $bootsize
		bootsize=`wc -c < ../BOOT/ZBDO230112V3010.bin`
		let bootsize=0x60000-bootsize
		#dd if=/dev/zero of=first.bin bs=$bootsize count=1
		#let bootsize=bootsize-1
		tr '\000' '\377' < /dev/zero | dd of=first.bin bs=1 count=$bootsize
		cat ../BOOT/ZBDO230112V3010.bin first.bin >dest.bin
		cat dest.bin $ver_cco.bin >$ver_cco_c.bin
		rm first.bin
		rm dest.bin
		echo "Combine $ver_sta_233l_c.bin success!"
	else
		echo " ZBDO230112V3010.bin is not exists, please add boot ZBDO230112V3010.bin in ../BOOT"
		exit
	fi

fi
if [ ! -f "$myPath/$ver_sta.bin" ];then
	echo "start move $ver_sta.bin!"
	mv $ver_sta.bin  $myPath/$ver_sta.bin
else
	echo "$ver_sta.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_cjq2.bin" ];then
	echo "start move $ver_cjq2.bin!"
	mv $ver_cjq2.bin  $myPath/$ver_cjq2.bin
else
	echo "$ver_cjq2.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_cco.bin" ];then
	echo "start move $ver_cco.bin!"
	mv $ver_cco.bin  $myPath/$ver_cco.bin
else
	echo "$ver_cco.bin has been exists!"
fi
if [ ! -f "$myPath/$ver_sta_c.bin" ];then
	echo "start move $ver_sta_c.bin!"
	mv $ver_sta_c.bin  $myPath/$ver_sta_c.bin
else
	echo "$ver_sta_c.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_cjq2_c.bin" ];then
	echo "start move $ver_cjq2_c.bin!"
	mv $ver_cjq2_c.bin  $myPath/$ver_cjq2_c.bin
else
	echo "$ver_cjq2_c.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_cco_c.bin" ];then
	echo "start move $ver_cco_c.bin!"
	mv $ver_cco_c.bin  $myPath/$ver_cco_c.bin
else
	echo "$ver_cco_c.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_sta_riscv.bin" ];then
	echo "start move $ver_sta_riscv.bin!"
	mv $ver_sta_riscv.bin  $myPath/$ver_sta_riscv.bin
else
	echo "$ver_sta_riscv.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_sta_riscv_c.bin" ];then
	echo "start move $ver_sta_riscv_c.bin!"
	mv $ver_sta_riscv_c.bin  $myPath/$ver_sta_riscv_c.bin
else
	echo "$ver_sta_riscv_c.bin has been exists!"
fi
if [ ! -f "$myPath/$ver_sta_233l.bin" ];then
	echo "start move $ver_sta_233l.bin!"
	mv $ver_sta_233l.bin  $myPath/$ver_sta_233l.bin
else
	echo "$ver_sta_233l.bin has been exists!"
fi

if [ ! -f "$myPath/$ver_sta_233l_c.bin" ];then
	echo "start move $ver_sta_233l_c.bin!"
	mv $ver_sta_233l_c.bin  $myPath/$ver_sta_233l_c.bin
else
	echo "$ver_sta_233l_c.bin has been exists!"
fi
if [ ! -f "$myPath/$ver_cco_233l.bin" ];then
	echo "start move $ver_cco_233l.bin!"
	mv $ver_cco_233l.bin  $myPath/$ver_cco_233l.bin
else
	echo "$ver_cco_233l.bin has been exists!"
fi
if [ ! -f "$myPath/$ver_cco_233l_c.bin" ];then
	echo "start move $ver_cco_233l_c.bin!"
	mv $ver_cco_233l_c.bin  $myPath/$ver_cco_233l_c.bin
else
	echo "$ver_cco_233l_c.bin has been exists!"
fi
rm -rf .version.sh.sw*

for file in $myPath*

do
	echo $file
done


