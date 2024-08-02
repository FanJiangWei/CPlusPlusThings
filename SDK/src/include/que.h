/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	que.h
 * Purpose:	Message queue manipulation structures.
 * History:
 *	Jul 15, 2007	jetmotor	create
 */

#ifndef _QUE_H
#define _QUE_H

#include "trios.h"
#include "list.h"


/* msgq_t - Message queue that acts as a data pipe between tasks and ISRs
 * @nr: amount of messages
 * @off: offset where a `list_head_t' linkage starts within a message
 * @thd: threshold, the maximum amount of messages a `msgq_t' can contain
 * @head: list head for all the messages
 */
typedef struct msgq_s {
	event_t cond;

	int32_t nr;
	int32_t off;
	int32_t thd;
	list_head_t head;
} msgq_t;

#define msgq_first(q)	        (void *)((char *)(q)->head.next - (q)->off)

/* msgq_init - Initialize a message queue.
 * @q: the message queue
 * @name: queue name
 * @off: offset where a `list_head_t' linkage starts within a message
 * @thd: threshold, the maximum amount of messages a `msgq_t' can contain
 * @return: -EBUSY if failed
 *          0 if succeed
 */
extern int32_t msgq_init(msgq_t *q, char *name, int32_t off, int32_t thd);

/* msgq_enq - Enque a message into queue's tail, and drop the oldest(first) message if the threshold
 *            will be exceeded.
 * @q: the queue
 * @obj: the message
 * @return: NULL if the threshold havn't been reached
 *          non-NULL, the dropped message
 */
extern void * msgq_enq(msgq_t *q, void *obj);

/* msgq_deq - Deque a message from queue's head. If there is no message any more, the calling task
 *            will be blocked until there's at least one.
 * @q: the queue
 * @return: the message
 */
extern void * msgq_deq(msgq_t *q);


static __inline__ int32_t msgq_is_empty(msgq_t *q)
{
	return list_empty(&q->head);
}

static __inline__ void *msgq_next(msgq_t *q, void *obj)
{
	list_head_t *pos;

	pos = (list_head_t *)(obj + q->off);
	return pos->next != &q->head ? ((void *)pos->next - q->off) : NULL;
}

static __inline__ void msgq_del(msgq_t *q, void *obj)
{
	list_head_t *pos;

	pos = (list_head_t *)(obj + q->off);
	list_del(pos);
	--q->nr;
	return;
}


#endif	/* end of _QUE_H */
