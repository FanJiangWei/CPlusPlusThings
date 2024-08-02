#ifndef _SHA1_H_
#define _SHA1_H_
#include <stdint.h>
#include "version.h"
#include "app.h"



#ifdef CMP_VERSION

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
	shaSuccess = 0,
	shaNull,            /* Null pointer parameter */
	shaInputTooLong,    /* input data too long */
	shaStateError       /* called Input after Result */
};
#endif
#define SHA1HashSize 20

/*
*  This structure will hold context information for the SHA-1
*  hashing operation
*/
typedef struct SHA1Context
{
	uint32_t Intermediate_Hash[SHA1HashSize / 4]; /* Message Digest  */

	uint32_t Length_Low;            /* Message length in bits      */
	uint32_t Length_High;           /* Message length in bits      */

	/* Index into message block array   */
	int16_t Message_Block_Index; //int_least16_t
	uint8_t Message_Block[64];      /* 512-bit message blocks      */

	int Computed;               /* Is the digest computed?         */
	int Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

/*
*  Function Prototypes
*/
int SHA1Reset(SHA1Context *context);
int SHA1Input(SHA1Context    *context,const uint8_t *message_array,unsigned length);
int SHA1Result(SHA1Context *context,uint8_t Message_Digest[SHA1HashSize]);
#endif
#endif

