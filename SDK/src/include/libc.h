/*
 * Copyright: (c) 2006-2022, Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	libc.h
 * Purpose:	A lite version C library developed for the embedded environment
 * History:
 *	Apr 30, 2022	jetmotor	create
 */

#ifndef	__LIBC_H__
#define __LIBC_H__

#ifndef	TRLIBC
#define TRLIBC 1 
#endif

extern int __strdcmp(const char *s1, const char *s2, char delimit);
extern int __strdbeg(const char *s, const char *t, char delimit);

/* The implementation of the C libary relies heavliy on system calls or functionalities
 * provided by the underlying operating system, which is by default considered to be a
 * fully-functional, sophisticated operating system such as Linux or Windows that supports
 * file systems, wide characters. dynamic memory allocation and reclaiming, etc.. However,
 * TRIOS, a RTOS in fact running in the embedded environment, is not such a case. Therefore,
 * most of the functions in certain standard C library (i.e., glibc or newlibc) don't fit
 * TRIOS very well if they're not augmented appropriately and carefully. Particularly, I
 * choose to reimplement parts of the C library to emphasize the following considerations:
 *	* as simple as possible, which comes to less memory consumption (saving 40+ KBytes
 *	  for ARM cortex M3/M4 or 60+ KBytes for RISC-V);
 *	* as fast as possible, which implies less subroutine invocation depths, and which
 *	  comes to less stack(memory) consumption eventually;
 *	* as safe as possbile, which implies building thread safe C functions directly on
 *	  the TRIOS synchronization primitives;
 *
 * It's strongly recommended to use functions declared in this header file instead of
 * those counterparts in the standard library as to reduce the risk of stack deficiency
 * of certain SDK tasks.
 */
#if TRLIBC
#include <stddef.h>
extern void * __memcpy(void *dst, const void *src, size_t len);
extern void * __memchr(void *s, int c, size_t n);
extern void * __memmove(void *dest, const void *src, size_t n);
extern char * __strncpy(char *dest, const char *src, size_t n);
extern int __strcmp(const char *s1, const char *s2);
extern int __strncmp(const char *s1, const char *s2, size_t n);
extern size_t __strlen(const char *s);
extern char * __strchr(char *s, int c);
extern char * __strstr(char *haystack, char *needle);
extern char * __strdup(const char *s);

extern unsigned long int __strtoul(char *nptr, char **endptr, int base);
extern long int __strtol(char *nptr, char **endptr, int base);
extern double __strtod(char *nptr, char **endptr);
extern int __atoi(char *nptr);
extern double __atof(char *nptr);
extern void * __malloc(size_t size);
extern void __free(void *ptr);

#include <stdarg.h>
extern int __vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
extern int __snprintf(char *str, size_t size, const char *fmt, ...);
extern int __sscanf(char *str, const char *fmt, ...);

#include <stdlib.h>
#else

#include <string.h>
#define __memcpy	memcpy
#define __memchr	memchr
#define __memmove	memmove
#define __strncpy	strncpy
#define __strcmp	strcmp
#define __strncmp	strncmp
#define __strlen	strlen
#define __strchr	strchr
#define __strstr	strstr
#define __strdup	strdup

#include <stdlib.h>
#define __strtoul	strtoul
#define __strtol	strtol
#define __strtod	strtod
#define __atoi		atoi
#define __atof		atof
#define __malloc	malloc
#define __free		free

#include <stdio.h>
#define __vsnprintf	vsnprintf
#define __snprintf	snprintf
#define __sscanf	sscanf
#endif	/* end of TRLIBC */


#endif	/* end of __LIBC_H__ */
