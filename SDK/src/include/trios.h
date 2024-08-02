/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	trios.h
 * Purpose:	OS APIs
 * History:
 *	Jan 1, 2010	jetmotor	Create
 */

#ifndef __TRIOS_H__
#define __TRIOS_H__


#include "types.h"
#include "list.h"
#include "cpu.h"


#if !defined(__UNIX_ERROR_NUM__)
#define EPERM		 1	/* Operation not permitted */
#define ENOENT		 2	/* No such file or directory */
#define ESRCH		 3	/* No such process */
#define EINTR		 4	/* Interrupted system call */
#define EIO		 5	/* I/O error */
#define ENXIO		 6	/* No such device or address */
#define E2BIG		 7	/* Argument list too long */
#define ENOEXEC		 8	/* Exec format error */
#define EBADF		 9	/* Bad file number */
#define ECHILD		10	/* No child processes */
#define EAGAIN		11	/* Try again */
#define ENOMEM		12	/* Out of memory */
#define EACCES		13	/* Permission denied */
#define EFAULT		14	/* Bad address */
#define ENOTBLK		15	/* Block device required */
#define EBUSY		16	/* Device or resource busy */
#define EEXIST		17	/* File exists */
#define EXDEV		18	/* Cross-device link */
#define ENODEV		19	/* No such device */
#define ENOTDIR		20	/* Not a directory */
#define EISDIR		21	/* Is a directory */
#define EINVAL		22	/* Invalid argument */
#define ENFILE		23	/* File table overflow */
#define EMFILE		24	/* Too many open files */
#define ENOTTY		25	/* Not a typewriter */
#define ETXTBSY		26	/* Text file busy */
#define EFBIG		27	/* File too large */
#define ENOSPC		28	/* No space left on device */
#define ESPIPE		29	/* Illegal seek */
#define EROFS		30	/* Read-only file system */
#define EMLINK		31	/* Too many links */
#define EPIPE		32	/* Broken pipe */
#define EDOM		33	/* Math argument out of domain of func */
#define ERANGE		34	/* Math result not representable */
#define EDEADLK		35	/* Resource deadlock would occur */
#define ENAMETOOLONG	36	/* File name too long */
#define ENOLCK		37	/* No record locks available */
#define ENOSYS		38	/* Function not implemented */
#define ENOTEMPTY	39	/* Directory not empty */
#define ELOOP		40	/* Too many symbolic links encountered */
#define EWOULDBLOCK	EAGAIN	/* Operation would block */
#define ENOMSG		42	/* No message of desired type */
#define EIDRM		43	/* Identifier removed */
#define ECHRNG		44	/* Channel number out of range */
#define EL2NSYNC	45	/* Level 2 not synchronized */
#define EL3HLT		46	/* Level 3 halted */
#define EL3RST		47	/* Level 3 reset */
#define ELNRNG		48	/* Link number out of range */
#define EUNATCH		49	/* Protocol driver not attached */
#define ENOCSI		50	/* No CSI structure available */
#define EL2HLT		51	/* Level 2 halted */
#define EBADE		52	/* Invalid exchange */
#define EBADR		53	/* Invalid request descriptor */
#define EXFULL		54	/* Exchange full */
#define ENOANO		55	/* No anode */
#define EBADRQC		56	/* Invalid request code */
#define EBADSLT		57	/* Invalid slot */
#define EDEADLOCK	EDEADLK
#define EBFONT		59	/* Bad font file format */
#define ENOSTR		60	/* Device not a stream */
#define ENODATA		61	/* No data available */
#define ETIME		62	/* Timer expired */
#define ENOSR		63	/* Out of streams resources */
#define ENONET		64	/* Machine is not on the network */
#define ENOPKG		65	/* Package not installed */
#define EREMOTE		66	/* Object is remote */
#define ENOLINK		67	/* Link has been severed */
#define EADV		68	/* Advertise error */
#define ESRMNT		69	/* Srmount error */
#define ECOMM		70	/* Communication error on send */
#define EPROTO		71	/* Protocol error */
#define EMULTIHOP	72	/* Multihop attempted */
#define EDOTDOT		73	/* RFS specific error */
#define EBADMSG		74	/* Not a data message */
#define EOVERFLOW	75	/* Value too large for defined data type */
#define ENOTUNIQ	76	/* Name not unique on network */
#define EBADFD		77	/* File descriptor in bad state */
#define EREMCHG		78	/* Remote address changed */
#define ELIBACC		79	/* Can not access a needed shared library */
#define ELIBBAD		80	/* Accessing a corrupted shared library */
#define ELIBSCN		81	/* .lib section in a.out corrupted */
#define ELIBMAX		82	/* Attempting to link in too many shared libraries */
#define ELIBEXEC	83	/* Cannot exec a shared library directly */
#define EILSEQ		84	/* Illegal byte sequence */
#define ERESTART	85	/* Interrupted system call should be restarted */
#define ESTRPIPE	86	/* Streams pipe error */
#define EUSERS		87	/* Too many users */
#define ENOTSOCK	88	/* Socket operation on non-socket */
#define EDESTADDRREQ	89	/* Destination address required */
#define EMSGSIZE	90	/* Message too long */
#define EPROTOTYPE	91	/* Protocol wrong type for socket */
#define ENOPROTOOPT	92	/* Protocol not available */
#define EPROTONOSUPPORT	93	/* Protocol not supported */
#define ESOCKTNOSUPPORT	94	/* Socket type not supported */
#define EOPNOTSUPP	95	/* Operation not supported on transport endpoint */
#define EPFNOSUPPORT	96	/* Protocol family not supported */
#define EAFNOSUPPORT	97	/* Address family not supported by protocol */
#define EADDRINUSE	98	/* Address already in use */
#define EADDRNOTAVAIL	99	/* Cannot assign requested address */
#define ENETDOWN	100	/* Network is down */
#define ENETUNREACH	101	/* Network is unreachable */
#define ENETRESET	102	/* Network dropped connection because of reset */
#define ECONNABORTED	103	/* Software caused connection abort */
#define ECONNRESET	104	/* Connection reset by peer */
#define ENOBUFS		105	/* No buffer space available */
#define EISCONN		106	/* Transport endpoint is already connected */
#define ENOTCONN	107	/* Transport endpoint is not connected */
#define ESHUTDOWN	108	/* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS	109	/* Too many references: cannot splice */
#define ETIMEDOUT	110	/* Connection timed out */
#define ECONNREFUSED	111	/* Connection refused */
#define EHOSTDOWN	112	/* Host is down */
#define EHOSTUNREACH	113	/* No route to host */
#define EALREADY	114	/* Operation already in progress */
#define EINPROGRESS	115	/* Operation now in progress */
#define ESTALE		116	/* Stale NFS file handle */
#define EUCLEAN		117	/* Structure needs cleaning */
#define ENOTNAM		118	/* Not a XENIX named type file */
#define ENAVAIL		119	/* No XENIX semaphores available */
#define EISNAM		120	/* Is a named type file */
#define EREMOTEIO	121	/* Remote I/O error */
#define EDQUOT		122	/* Quota exceeded */
#define ENOMEDIUM	123	/* No medium found */
#define EMEDIUMTYPE	124	/* Wrong medium type */

#ifndef errno
#define errno (*__errno())
#endif
#endif


#if !defined(__OS_SCHED_H__)
typedef struct queue_s {
	uint32_t nr;
	list_head_t hdr;
} queue_t;

#define queue_empty(q)	list_empty(&(q)->hdr)
#define queue_first(q)	((q)->hdr.next)
#define queue_last(q)	((q)->hdr.prev)



extern void queue_init(queue_t *q);
/* queue_enq_tail - Put object `@obj' into queue's tail.
 * @q:
 * @obj:
 */
extern void queue_enq_tail(queue_t *q, list_head_t *obj);
/* queue_enq_head - Put object `@obj' into queue's head.
 * @q:
 * @obj:
 */
extern void queue_enq_head(queue_t *q, list_head_t *obj);
/* queue_enq_list - Put a `@list' of `@nr' objects into queue's tail.
 * @q:
 * @list:
 * @nr:
 */
extern void queue_enq_list(queue_t *q, list_head_t *list, int32_t nr);
/* queue_del - Delete `@obj' from `q'.
 * @q:
 * @obj:
 */
extern void queue_del(queue_t *q, list_head_t *obj);
struct tty_s;
extern void trios_pool(struct tty_s *term);
#endif


#if !defined(__OS_TASK_H__)
typedef struct task_s {
	uint32_t	*stack;
	uint32_t	*_stack;
	uint32_t	size;

	uint16_t	pid;
	uint8_t		state;
	uint8_t		timeout;

	list_head_t	link;
	list_head_t	tvlink;
	queue_t		*from;
	uint32_t	dy_prio;
	uint32_t	prio;
	uint32_t	time_slice;
	uint32_t	delay;
	uint16_t	x;
	uint16_t	y;
	uint32_t	timestamp;
	uint32_t	sleep_avg;
	uint32_t	run_time;
	char		*name;
} task_t;

/* CURRENT - Points to the current running task.
 * Note: read only global variable, never try to modify it.
 */
extern task_t *CURRENT;
/* OSIntNesting - trios interrupt nesting depth.
 * 0 for task context, 1 for non-nested interrupt context, > 1 for nested interrupt context.
 * Note: read only global variable, never try to modify it.
 */
extern uint32_t OSIntNesting;

/* task_create - Create a new trios task.
 * @func: task's main function, which must be an infinite loop
 * @arg: main function's argument
 * @sp: top (the highest address) of task's stack, 4-byte aligned address
 * @prio: task's priority, 0 ~ 63, less number, higher priority
 * @name: task's name
 * @_sp: bottom (the lowest address) of task's stack, 4-byte aligned address
 * @size: task stack's size
 * @return:  -EINVAL if arguments error,
 *           -ENOBUFS if no more task vacancy,
 *           otherwise task's id (a non-zero positive integer)
 */
extern int32_t task_create(void (*func)(void *arg), void *arg, uint32_t *sp, uint32_t prio, char *name, uint32_t *_sp, uint32_t size);
#if !(defined(LIGHTELF2M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(UNICORN2M))
extern int32_t task_create_easy(void (*func)(void *arg), void *arg, uint32_t *sp, uint32_t prio, char *name);
#endif
/* task_delete - Delete an already created trios task.
 * @pid: task's pid
 * @return: -EINVAL if `@pid' wrong,
 *          0 if succeed
 */
extern int32_t task_delete(uint32_t pid);
#if !(defined(LIGHTELF2M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(UNICORN2M))
/* task_exit - Run to end for a task itself.
 * @return: NEVER return
 */
extern int32_t task_exit(void);
/* task_suspend - Suspend an already created trios task, no matter it's running or waiting.
 * @pid: task's pid
 * @return: -EINVAL if `@pid' wrong,
 *          0 if succeed
 */
extern int32_t task_suspend(uint32_t pid);
/* task_resume - Resume an already suspended trios task.
 * @pid: task's pid
 * @return: -EINVAL if `@pid' wrong,
 *          0 if succeed
 */
extern int32_t task_resume(uint32_t pid);
#endif
/* task_unused_stack - Estimate the unused stack size of an already created task.
 * Note: Only tasks created by `task_create' are able to have stack estimated
 * @pid: task's pid
 * @return: -EINVAL if bottom of task stack unknown, 
 *          otherwise, estimated unused stack size, in unit of sizeof(uint32_t)
 */
extern int32_t task_unused_stack(uint32_t pid);
/* task_query - Get pid of the task who invokes this API.
 * @return: task's pid
 */
extern uint32_t task_query(void);
/* task_get - Get task instance's handler of certain `@pid'.
 * @pid: task's pid
 * @return: task instance's handler
 */
extern task_t * task_get(uint32_t pid);
/* task_dump: Display miscellanenous task information.
 * @pid: task's pid
 * @term: tty or dty
 * @return: -EINVAL if `@pid' wrong,
 *          0 if succeed
 */
struct tty_s;
extern int32_t task_dump(uint32_t pid, struct tty_s *term);
#endif


#if !defined(__OS_EVENT_H__)
typedef struct event_s {
	list_head_t	link;
	queue_t		waitq;
	int32_t		count;
	uint32_t	type;
	char		*name;
} event_t;


enum event_type {
	EVENT_SEM = 0,
	EVENT_MUTEX,
	EVENT_CONDITION,
	EVENT_NULL
};
#endif


#if !defined(__OS_Q_H__)
typedef struct os_q_s {
	event_t cond;

	void **base;
	uint32_t front;
	uint32_t rear;
	uint32_t size;
} os_q_t;

/* os_q_init - Initialize a trios queue with finit capacity.
 * @q: the queue
 * @base: an array of void *, wherein each element represents a queue entry
 * @size: size of `@base', i.e. queue's capacity
 * @name: queue's name
 */
extern void os_q_init(os_q_t *q, void **base, uint32_t size, char *name);
/* os_q_fina - Finalize a trios queue without considering how to handle the messages if there's any.
 * @q: the queue
 */
extern void os_q_fina(os_q_t *q);
/* os_q_pend - Any tasks that try to pend a message from a queue will:
 *            1. return immediately with a valid message;
 *            2. wait forever until there's at least one message in queue;
 *            3. wait until timeout (in unit of `MS_PER_OS_TICKS' ms).
 * @q: the queue
 * @timeout: 0 if tasks wait forever,
 *           non-zero if tasks wait for `@timeout' at most
 * @err: error code, -ENOMSG when queue empty
 * @return: NULL if arguments error or queueu empty (with error code -ENOMSG in `@err')
 *          the message
 */
extern void * os_q_pend(os_q_t *q, uint32_t timeout, int8_t *err);
/* os_q_post - Try to post a message into queue.
 * @q: the queue
 * @msg: the message
 * @return: -EINVAL if arguments error
 *          -ENOENT if queue is full
 *          0 if succeed
 */
extern int32_t os_q_post(os_q_t *q, void *msg);
/* os_q_flush - Drain the whole queue simply without considering how to handle the messages if there's any.
 * @q: the queue
 */
extern void os_q_flush(os_q_t *q);
#endif


#if !defined(__OS_WAIT_H__)
/* sem_init - Initialize a trios semaphore for task synchronization.
 * @evt: the semaphore
 * @count: semaphore's initial count
 * @name: semaphore's name
 */
extern void sem_init(event_t *evt, uint32_t count, char *name);
/* sem_fina - Finalize a trios semaphore initialized by `sem_init' and wake up all the tasks pending on it.
 * &evt: the semaphore
 */
extern void sem_fina(event_t *evt);
/* sem_pend - Any tasks that try to pend on a semaphore will:
 *            1. return immediately if the semaphore have sufficient resources;
 *            2. wait forever until the semaphore become available;
 *            3. wait until timeout (in unit of `MS_PER_OS_TICKS' ms).
 * @evt: the semaphore
 * @timeout: 0 if tasks wait forever,
 *           non-zero if tasks wait for `@timeout' at most
 * @return: -EINVAL if argument error or used in ISR context,
 *           0 if succeed,
 *           1 if task's waked up due to timeout eventually
 */
extern int32_t sem_pend(event_t *evt, uint32_t timeout);
/* sem_accept - Try to acquire a semaphore if it's available.
 * @evt: the semaphore
 * @return: 0 if the semaphore is not available,
 *          non-zero if acquire the semaphore successfully
 */
extern int32_t sem_accept(event_t *evt);
/* sem_post: Try to post a sempahore and wake up the awaiting task with the highest priority if there's any.
 * @evt: the semaphore
 * @return: -EINVAL if argument error,
 *          0 if succeed
 */
extern int32_t sem_post(event_t *evt);

/* mutex_init - Initialize a trios mutex for task mutual exclusion.
 * @evt: the mutex
 * @name: mutex's name
 */
extern void mutex_init(event_t *evt, char *name);
/* mutex_fina - Finalize a trios semaphore initialized by `mutex_init' and wake up all the tasks pending on it.
 * &evt: the semaphore
 */
extern void mutex_fina(event_t *evt);
/* mutex_pend - Any tasks that try to pend on a mutex will:
 *              1. return immediately if the mutex is available;
 *              2. wait forever until the mutex become avaible;
 *              3. wait until timeout (in unit of `MS_PER_OS_TICKS' ms).
 * @evt: the mutex
 * @timeout: 0 if tasks wait forever,
 *           non-zero if tasks wait for `@timeout' at most
 * @return: -EINVAL if argument error or used in ISR context,
 *          0 if succeed, 
 *          1 if task's waked up due to timeout eventually
 */
extern int32_t mutex_pend(event_t *evt, uint32_t timeout);
/* mutex_post - Try to post a mutex and wake up the awaiting task with the highest priority if there's any.
 * @evt: the mutex
 * @return: -EINVAL if argument error,
 *          0 if succeed
 */
extern int32_t mutex_post(event_t *evt);
/* mutex_accept - Try to acquire a mutex if it's available.
 * @evt: the mutex
 * @return: 0 if the mutex is not available,
 *          non-zero if acquire the mutex successfully
 */
extern int32_t mutex_accept(event_t *evt);

/* condition_init - Initialize a trios condition variable for task synchronization
 * @evt: the condition variable
 * @name: condition variable's name
 */
extern void condition_init(event_t *evt, char *name);
/* condition_fina - Finalize a trios condition variable initialized by `condition_init' and wake up all the
 *                  tasks pending on it.
 * @evt: the condition variable
 */
extern void condition_fina(event_t *evt);
/* condition_post - Try to post a condition variable and wake up all the awaiting tasks if there's any.
 * @evt: the condition variable
 * @return: -EINVAL if argument error,
 *          0 if succeed
 */
extern int32_t condition_post(event_t *evt);
/* os_sleep - Sleep for a while.
 * @time: sleep time (in unit of `MS_PER_OS_TICKS' ms).
 * @return: -EINVAL if argument error,
 *          0 if succeed
 */
extern int32_t os_sleep(uint32_t time);
/* waiton_que_interruptible - For internal usage ONLY.
 */
extern void waiton_que_interruptible(queue_t *que, uint32_t timeout);
/* scheduler - Architecture dependent implementation to trigger a software interrupt manually.
 *             For internal usage ONLY.
 */
extern void scheduler(void);
/* condition_pend - Any tasks that try to pend on a condition will:
 *                  1. return immediately if `@condition' is tested as true;
 *                  2. wait forever until `@condition' is supposed to be true again;
 *                  3. wait until timeout (in unit of `MS_PER_OS_TICKS' ms).
 * @evt: the condition event
 * @to: 0 if tasks wait forever,
 *      non-zero if tasks wait for `@timeout' at most
 * @condition: a conditional expression
 * @return: -EINVAL if argument error or used in ISR context,
 *          0 if succeed,
 *          1 if task's waked up due to timeout eventually
 */
#define condition_pend(evt, to, condition)				\
({									\
	int32_t ret = 0;						\
									\
	do {								\
		uint32_t cpu_sr;					\
									\
		if (OSIntNesting) {					\
			ret = -EINVAL;					\
			break;						\
		}							\
									\
		cpu_sr = OS_ENTER_CRITICAL();				\
		if (condition) {					\
			waiton_que_interruptible(&(evt)->waitq, to);	\
			OS_EXIT_CRITICAL(cpu_sr);			\
			scheduler();					\
			cpu_sr = OS_ENTER_CRITICAL();			\
			ret = CURRENT->timeout;				\
			if (CURRENT->timeout)				\
				CURRENT->timeout = 0;			\
		}							\
		OS_EXIT_CRITICAL(cpu_sr);				\
	} while (0);							\
									\
	ret;								\
})
#endif


#if !defined(__OS_TIME_H__)
#define TMR_NULL		0
#define TMR_PERIODIC		(1 << 0)
#define TMR_TRIGGER		(1 << 1)
#define TMR_CALLBACK		(1 << 2)
#define TMR_REALTIME		(1 << 3)

#define TMR_STOPPED		0
#define TMR_RUNNING		1

typedef struct ostimer_s {
	list_head_t link;
	uint32_t option;
	uint32_t state;
	void (*fn)(struct ostimer_s *, void *);
	void *arg;
	char *name;
	struct {
		uint32_t x, y;
		uint32_t expires;
	} two[2];
} ostimer_t;

/* timer_init - Initialize a trios timer with the highest resolution of `MS_PER_OS_TICKS' * `OS_TMR_HZ' = 10 * 1 = 10ms.
 * @first: the first timeout (in ms if !TMR_REALTIME, or in 1/CPU_CLK if TMR_REALTIME)
 * @interval: only for `TMR_PERIODIC' timer usage (in ms)
 * @opt: `TMR_PERIODIC' or `TMR_TRIGGER'
 * @fn: callback function invoked in TASK context when timer timeout
 * @arg: argument of `@fn'
 * @name: timer's name
 */
extern void timer_init(ostimer_t *tmr, uint32_t first, uint32_t interval, uint32_t opt, void (*fn)(struct ostimer_s *, void *), void *arg, char *name);
/* timer_fina - Finalize and stop a trios timer.
 * @tmr: the timer
 */


extern void *zc_malloc_mem(uint16_t size,char *s,uint16_t time);
extern uint8_t zc_free_mem(void *p);
extern ostimer_t *timer_create(uint32_t first, uint32_t interval, uint32_t opt, void (*fn)(struct ostimer_s *, void *), void *arg, char *name);

extern  void timer_delete(ostimer_t *tmr);

extern void timer_fina(ostimer_t *tmr);
/* timer_start - Start timer counting immediately.
 * @tmr: the timer
 * @return: -EINVAL if arguemnt error
 *          0 if succeed
 */
extern int32_t timer_start(ostimer_t *tmr);
/* timer_stop - Stop timer counting immediately and invoke the callback function if `@opt' is `TMR_CALLBACK'.
 * @tmr: the timer
 * @opt: `TMR_NULL' or `TMR_CALLBACK'
 * @return: -EINVAL if arguments error,
 *          0 if succeed
 */
extern int32_t timer_stop(ostimer_t *tmr, uint32_t opt);
/* timer_modify - Modify an already stopped timer's features unless the `force' flag is set as to stop the timer in priori
 *                if it's running then.
 * @tmr: the timer
 * @first: the first timeout (in ms if !TMR_REALTIME, or in 1/CPU_CLK if TMR_REALTIME)
 * @interval: only for `TMR_PERIODIC' timer usage (in ms)
 * @opt: `TMR_PERIODIC' or `TMR_TRIGGER'
 * @fn: callback function invoked in TASK context when timer timeout
 * @arg: argument of `@fn'
 * @name: timer's name
 * @force: force to modify a timer even when it's running
 * @return: -EINVAL if argument error
 *          -EBUSY if the timer's running without setting `force'
 *          0 if succeed
 */
extern int32_t timer_modify(ostimer_t *tmr, uint32_t first, uint32_t interval, uint32_t opt, void (*fn)(ostimer_t *tmr, void *arg), void *arg, char *name,
			    int32_t force);
/* timer_query - Query the timer's state, namely, `TMR_STOPPED' or `TMR_RUNNING'.
 * @tmr: the timer
 * @return: -EINVAL if argument error,
 *          `TMR_STOPPED' or `TMR_RUNNING'
 */

extern int32_t timer_query(ostimer_t *tmr);


#define zc_timer_query(timer)   \
({  \
    uint32_t tmp;       \
    if(timer == NULL)   \
        {tmp = TMR_STOPPED; /*printf_s("Timer is null!\n");*/}\
    else    \
        tmp = timer_query(timer);  \
        \
        tmp;\
})

/* timer_remain - 
 * @tmr:
 * @return: (in ms)
 */
extern uint32_t timer_remain(ostimer_t *tmr);
/* os_ticks - Get trios ticks with resolution of `MS_PER_OS_TICKS' since it starts up.
 * @return: OS ticks
 */
extern uint32_t os_ticks(void);
/* os_ms - Get trios milli-seconds with resolution of `MS_PER_OS_TICKS' since it starts up.
 * @return: OS milli-seconds
 */
extern uint32_t os_ms(void);
/* os_us - Get trios micro-seconds with resolution of `MS_PER_OS_TICKS' since it starts up.
 * @return: OS micro-seconds
 */
extern uint32_t	os_us(void);
/* os_us - Get trios seconds since it starts up.
 * @return: OS seconds
 */
extern uint32_t os_time(void);
/*
 * os_set_time - Set system seconds.
 * @return: System seconds since 19700101
 */
extern uint32_t os_set_time(uint32_t secs);
/* os_ms2tick - Transform milli-seconds to trios ticks.
 * @ms: milli-seconds
 * @return: trios ticks
 */
extern uint32_t os_ms2tick(uint32_t ms);
/* os_rtc_time - get rtc time seconds,
 * @return: if sync rtc time, return rtc time seconds since 19700101; otherwise return 0
 * */
extern uint32_t os_rtc_time(void);
/* os_ticks - Get rtc ticks with resolution of `MS_PER_OS_TICKS' since it rtc time run
 * @return: rtc ticks
 */
extern uint32_t os_rtc_ticks(void);
#endif


#if !defined(__OS_MEM_H__)
typedef struct pool_s {
	list_head_t link;	/* link to os memory mangement list */

	list_head_t free;

	char *name;
	uint32_t nr;
	uint32_t sz;

	uint32_t nr_free;
	uint32_t nr_used;
	uint32_t min_free;
} pool_t;

/* create_pool - Create a trios pool for buffer management.
 * @pool: the pool
 * @buf: the buffer
 * @nr: number of units
 * @sz: unit size (in bytes)
 * @off: offset (4-bytes aligned) within the unit for buffer management overhead,
 *       the remaining space after offset should be no less than 8 bytes
 * @name: pool name
 */
extern void create_pool(pool_t *pool, void *buf, uint32_t nr, uint32_t sz, int32_t off, char *name);
/* get_from_pool - Atomically get a buffer unit from pool.
 * @pool: the pool
 * @off: offset within the unit for buffer management overhead
 * @return: NULL if no more buffer unit vacancy
 *          handler of the buffer unit
 */
extern void * get_from_pool(pool_t *pool, int32_t off);
/* get_from_pool_easy - Non-atomically get a buffer unit from pool.
 * @pool: the pool
 * @off: offset within the unit for buffer management overhead
 */
extern void * get_from_pool_easy(pool_t *pool, int32_t off);
/* put_into_pool - Atomically reclaim a buffer unit into pool.
 * @pool: the pool
 * @ent: the buffer unit
 * @off: offset within the unit for buffer management overhead
 */
extern void put_into_pool(pool_t *pool, void *ent, int32_t off);
/* put_into_pool_easy - Non-atomically reclaim a buffer unit into pool.
 * @pool: the pool
 * @ent: the buffer unit
 * @off: offset within the unit for buffer management overhead
 */
extern void put_into_pool_easy(pool_t *pool, void *ent, int32_t off);
/* put_into_pool_list - Atomically reclaim a list of buffer units into pool.
 * @pool: the pool
 * @list: head of the list
 * @nr: number of to-be-reclaimed buffer units
 */
extern void put_into_pool_list(pool_t *pool, list_head_t *list, int32_t nr);
/* put_into_pool_list_easy - Non-atomically reclaim a list of buffer units into pool.
 * @pool: the pool 
 * @list: head of the list
 * @nr: number of to-be-reclaimed buffer units
 */
extern void put_into_pool_list_easy(pool_t *pool, list_head_t *list, int32_t nr);
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


#endif	/* end of __TRIOS_H__ */

