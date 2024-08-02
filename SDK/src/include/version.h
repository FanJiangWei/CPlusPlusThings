#ifndef __VERSION_H
#define __VERSION_H

#define VERSION		"ver_03-02-12-03"


#define VENDORSIZE  16


#if defined(STD_2016) || defined(STD_DUAL)

#if defined(CHIP_R)

#define ZC3951CCO_ver1 0x60
#define ZC3951CCO_ver2 0x00
#define ZC3951CCO_chip1 'D'
#define ZC3951CCO_chip2 'N'
#define ZC3951CCO_Innerver1 0x60
#define ZC3951CCO_Innerver2 0x00

#define ZC3750CJQ2_ver1 0x60
#define ZC3750CJQ2_ver2 0x00
#define ZC3750CJQ2_chip1  'h'
#define ZC3750CJQ2_chip2  'n'
#define ZC3750CJQ2_Innerver1 0x60
#define ZC3750CJQ2_Innerver2 0x00


#define ZC3750STA_ver1 0x60
#define ZC3750STA_ver2 0x00
#define ZC3750STA_chip1  'H'
#define ZC3750STA_chip2	 'n'
#define ZC3750STA_Innerver1 0x60
#define ZC3750STA_Innerver2 0x00


#elif defined(CHIP_U)

#define ZC3951CCO_ver1 0x21
#define ZC3951CCO_ver2 0x11
#define ZC3951CCO_chip1 'D'
#define ZC3951CCO_chip2 'N'
#define ZC3951CCO_Innerver1 0X21
#define ZC3951CCO_Innerver2 0x11


#define ZC3750CJQ2_ver1 0x40
#define ZC3750CJQ2_ver2 0x09
#define ZC3750CJQ2_chip1  'h'
#define ZC3750CJQ2_chip2  'n'
#define ZC3750CJQ2_Innerver1 0x40
#define ZC3750CJQ2_Innerver2 0x09


#define ZC3750STA_ver1 0x21
#define ZC3750STA_ver2 0x11
#define ZC3750STA_chip1  'H'
#define ZC3750STA_chip2	 'n'
#define ZC3750STA_Innerver1 0X21
#define ZC3750STA_Innerver2 0x11

#elif defined(CHIP_D)

#define ZC3951CCO_ver1 0x10
#define ZC3951CCO_ver2 0x14
#define ZC3951CCO_chip1 'D'
#define ZC3951CCO_chip2 'O'
#define ZC3951CCO_Innerver1 0x10
#define ZC3951CCO_Innerver2 0x14

#define ZC3750CJQ2_ver1 0x10
#define ZC3750CJQ2_ver2 0x11
#define ZC3750CJQ2_chip1  'h'
#define ZC3750CJQ2_chip2  'o'
#define ZC3750CJQ2_Innerver1 0x10
#define ZC3750CJQ2_Innerver2 0x11

#define ZC3750STA_ver1 0x10
#define ZC3750STA_ver2 0x21
#define ZC3750STA_chip1  'I'
#define ZC3750STA_chip2	 'o'
#define ZC3750STA_Innerver1 0x10
#define ZC3750STA_Innerver2 0x21
#elif defined(CHIP_V)

#define ZC3951CCO_ver1 0x50
#define ZC3951CCO_ver2 0x01
#define ZC3951CCO_chip1 'D'
#define ZC3951CCO_chip2 'O'
#define ZC3951CCO_Innerver1 0x50
#define ZC3951CCO_Innerver2 0x01

#define ZC3750CJQ2_ver1 0x50
#define ZC3750CJQ2_ver2 0x01
#define ZC3750CJQ2_chip1  'h'
#define ZC3750CJQ2_chip2  'o'
#define ZC3750CJQ2_Innerver1 0x50
#define ZC3750CJQ2_Innerver2 0x01


#define ZC3750STA_ver1 0x50
#define ZC3750STA_ver2 0x01
#define ZC3750STA_chip1  'I'
#define ZC3750STA_chip2	 'o'
#define ZC3750STA_Innerver1 0x50
#define ZC3750STA_Innerver2 0x01

#endif
#elif defined(STD_GD)

#define ZC3951CCO_ver1 0xC0
#define ZC3951CCO_ver2 0x07
#define ZC3951CCO_chip1 'G'
#define ZC3951CCO_chip2 'N'


#define ZC3750CJQ2_ver1 0xC0
#define ZC3750CJQ2_ver2 0x07
#define ZC3750CJQ2_chip1  'p'
#define ZC3750CJQ2_chip2  'n'


#define ZC3750STA_ver1 0xC0
#define ZC3750STA_ver2 0x07
#define ZC3750STA_chip1  'S'
#define ZC3750STA_chip2	 'n'

#endif


// Inner device name and software vertion
#if defined(STD_2016) && (defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1_1M) || defined(ROLAND9_1M))

#define   PRODUCT_func              'P'
#define   CHIP_code                 'J'
#define   POWER_OFF                 'B'


#define   ZC3951CCO_type            'U'
#define   ZC3951CCO_prtcl           0x07
#define   ZC3951CCO_prdct           0x01
#define   ZC3951CCO_UpDate_type     'U'


#define   ZC3750STA_type            'M'
#define   ZC3750STA_prtcl           0x0F
#define   ZC3750STA_prdct           0x05
#define   ZC3750STA_UpDate_type     'M'


#define   ZC3750CJQ2_type           'C'
#define   ZC3750CJQ2_prtcl          0x0B
#define   ZC3750CJQ2_prdct          0x02
#define   ZC3750CJQ2_UpDate_type    'C'




#elif defined(STD_GD)

#define   PRODUCT_func   	        'P'
#define   CHIP_code                 'J'
#define   POWER_OFF                 'A'

#define   ZC3951CCO_type            'U'
#define   ZC3951CCO_prtcl           0x11
#define   ZC3951CCO_prdct  	        0x01
#define   ZC3951CCO_UpDate_type     'U'


#define   ZC3750STA_type            'M'
#define   ZC3750STA_prtcl           0x13
#define   ZC3750STA_prdct  	        0x05
#define   ZC3750STA_UpDate_type     'M'


#define   ZC3750CJQ2_type           'C'
#define   ZC3750CJQ2_prtcl          0x03
#define   ZC3750CJQ2_prdct 	        0x02
#define   ZC3750CJQ2_UpDate_type    'C'


#define   PRODUCT_func   	        'D'
#define   CHIP_code                 'B'
#define   POWER_OFF                 'B'


#define   ZC3951CCO_type            'U'
#define   ZC3951CCO_prtcl           0x07
#define   ZC3951CCO_prdct  	        0x01
#define   ZC3951CCO_UpDate_type     'u'


#define   ZC3750STA_type            'M'
#define   ZC3750STA_prtcl           0x0F
#define   ZC3750STA_prdct  	        0x05
#define   ZC3750STA_UpDate_type     'm'



#define   ZC3750CJQ2_prdct 	        0x02
#define   ZC3750CJQ2_UpDate_type    'c'

#elif defined(STD_DUAL) && (defined(VENUS2M) || defined(VENUS8M) || defined(MIZAR1M) || defined(MIZAR9M))

#define   PRODUCT_func   	        'D'
#define   CHIP_code                 'B'
#define   POWER_OFF                 'B'


#define   ZC3951CCO_type            'U'
#define   ZC3951CCO_prtcl           0x07
#define   ZC3951CCO_prdct           0x01
#define   ZC3951CCO_UpDate_type     'u'


#define   ZC3750STA_type            'Y'
#define   ZC3750STA_prtcl           0x00
#define   ZC3750STA_prdct           0x05
#define   ZC3750STA_UpDate_type     'y'


#define   ZC3750CJQ2_type           'C'
#define   ZC3750CJQ2_prtcl          0x0B
#define   ZC3750CJQ2_prdct          0x02
#define   ZC3750CJQ2_UpDate_type    'c'


#define   POWER_MANAGER

#endif

#define AREA_code                 0x00
#define PROPERTY                    'L'

#define bootversion                 1


#define Vender1                     'Z'
#define Vender2                     'C'


#define Date_Y 23
#define Date_M 3
#define Date_D 13

#define InnerDate_Y ((__DATE__[9] - '0') * 10 + __DATE__[10] - '0')
#define InnerDate_M (__DATE__[2] == 'n' ? (__DATE__[0] == 'J' ? (__DATE__[1] == 'a' ? 1 : (__DATE__[2] == 'n' ? 6 : 7)) : 12) : (__DATE__[2] == 'b' ? 2 : (__DATE__[2] == 'r' ? 4 : (__DATE__[2] == 'y' ? 2 : (__DATE__[2] == 'l' ? 7 : (__DATE__[2] == 'g' ? 8 : (__DATE__[2] == 'p' ? 9 : (__DATE__[2] == 't' ? 10 : (__DATE__[2] == 'v' ? 11 : 0)))))))))
#define InnerDate_D ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + __DATE__[5] - '0')


#endif
