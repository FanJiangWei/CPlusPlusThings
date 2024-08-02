#ifndef __APP_UPGRADE_COMM_H__
#define __APP_UPGRADE_COMM_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"

#define IMAGE_UNSAFE	                3
#define IMAGE_OK	                    2       /*image0 and image1 are all OK*/
#define IMAGE_ERROR	                    -1      /*image0 and image1 are all error*/

/* size of IMG_IDENT don't larger than SZ_IMG_IDENT - 1 */
//
#if defined(ROLAND1_1M) || defined(ROLAND9_1M) 
#define IMG_IDENT	"ZCHC3750A"
#define IMG_IDENT0	"triductor"

#elif defined(UNICORN2M) || defined(UNICORN8M)
#define IMG_IDENT	"triductor"
#define IMG_IDENT0	"ZCHC3750A"

#elif defined(VENUS2M) || defined(VENUS8M) || defined(MIZAR1M) || defined(MIZAR9M)
#if defined(V233L_3780)

#define IMG_IDENT	"ZCHC37801"
#define IMG_IDENT0	"triductor"
#define IMG_IDENT1	"ZCHC37800"
#define IMG_IDENT2	"ZCHC3750A"
#define IMG_IDENT3	"ZCHC37803"

#elif defined(RISCV)

#define IMG_IDENT	"ZCHC37800"
#define IMG_IDENT0	"triductor"
#define IMG_IDENT1	"ZCHC37801"
#define IMG_IDENT2	"ZCHC3750A"
#define IMG_IDENT3	"ZCHC37803"

#elif defined(VENUS_V3)

#define IMG_IDENT	"ZCHC37803"
#define IMG_IDENT0	"triductor"
#define IMG_IDENT1	"ZCHC37800"
#define IMG_IDENT2	"ZCHC3750A"
#define IMG_IDENT3	"ZCHC37801"

#else 

#define IMG_IDENT	"triductor"
#define IMG_IDENT0	"ZCHC37800"
#define IMG_IDENT1	"ZCHC37801"
#define IMG_IDENT2	"ZCHC3750A"
#define IMG_IDENT3	"ZCHC37803"

#endif

#endif


#define IMG_IDENT_TEST	                "ZCHCTESTL"
#define SZ_IMG_IDENT	                10
#define MAX_UPGRADE_FILE_SIZE           (MIN_UPGRADE_BYTE_SIZE * 1024)
#define FILE_TRANS_BLOCK_SIZE           400
#define MIN_FILE_TRANS_BLOCK_SIZE       100
#define UPGRADE_TIMEOUT                 180       // minitue
#define WAIT_RESET_TIME                 30        // Second
#define TEST_RUN_TIME                   0         // 180 Second
#define RETRANUPGRADEMAXNUM             5
#define UPGRADE_PLC_TIME_OUT            2000      //1000ms   unit:ms


typedef enum
{
    e_FILE_TRANS_UNICAST,
    e_UNICAST_TO_LOCAL_BROADCAST,
    e_FULL_NET_BROADCAST,
}FILE_TRANS_MODE_e;

typedef enum
{
    e_UPGRADESTART_SUCCESS,
    e_UPGRADE_BUSY,
} UPGRADESTART_RESULTCODE_e;

typedef enum
{
    e_INFO_ID_FACTORYCODE,
    e_INFO_ID_VERSION,
    e_INFO_ID_BOOTVER,
    e_INFO_ID_CRC32,
    e_INFO_ID_FILELENGTH,
    e_INFO_ID_DEVICETYPE,
}INFO_ID_e;

typedef struct imghdr_s {
    unsigned char	version[2];	            /* ww-xx */	
    unsigned char	chip;	                /* P */	
    unsigned char	devicetype;	            /* M C U O*/	
    unsigned char   ident[SZ_IMG_IDENT];	/* image identity */	
	//unsigned char   res[2];             	/* image identity */	
    unsigned char   ext_info_flag : 1;
    unsigned char                 : 7;
    unsigned char   res;
	unsigned char   outversion[2];	        /* image outversion */
	unsigned short  outdate;	            /* image outdate */
    unsigned int	crc;	
    unsigned int	sz_file;	            /* file size in byte including imghdr */	
    unsigned int	sz_ih;		            /* image header size in byte */	
    unsigned int	sz_sh;		            /* segment header table entry size */	
    unsigned int	nr_sh;		            /* segment header table entry count */
}__PACKED4 imghdr_t;

typedef struct seghdr_s {
	unsigned int start;		                /* load start address */
	unsigned int stop;		                /* load end address */
	unsigned int size;		                /* segment file size in bytes */
	unsigned int offset;		            /* segment file offset */
	unsigned int compress;		            /* segment compress is */
} seghdr_t;

typedef struct{
    U8      FactoryCode[2];
    U8      VerionInfo[2];
    U8      BootVer;
    U32     ImageCrc32;
    U32     ImageSize;
    U8      DeviceType;
}__PACKED CURR_SLA_STA_INFO_t;

typedef enum 
{
    e_UPGRADE_IDLE,
    e_FILE_DATA_RECEIVING,
    e_FILE_RCEV_FINISHED,
    e_UPGRADE_PERFORMING,
    e_UPGRADE_TEST_RUN,
}SLAVE_UPGRADE_STATUS_e;


U16  CheckFileTransFinish(U8  *pBitMapBuf, U16 totolBlockNum);
int32_t is_tri_image(imghdr_t *ih);
int32_t is_test_image(imghdr_t *ih);
U16  get_bit_state(U8* pBitMapBuf, U16 BlockSeq);
int32_t check_image(uint32_t addr, uint32_t sz);
uint8_t check_image_type(uint32_t addr, uint32_t sz);
int32_t check_image_block(uint32_t addr, uint32_t block_sz);
int32_t write_image_block(uint32_t addr, uint32_t block_sz);
void upgrade_show_bit_map(U8 *pBitMapBuf, U16 totolBlockNum);

#endif 

