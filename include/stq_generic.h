/*
 * vim:expandtab:shiftwidth=4:tabstop=4:
 *
 * Copyright   (2013)      Contributors
 * Contributor : chitr   chitr.prayatan@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ---------------------------------------
 */
/**
 * \file stq.h
 *
 * \brief
 */

#include <inttypes.h>

#ifndef _STQ_THREAD_H
#define _STQ_THREAD_H

#define MAX_CONTEXT_SIZE 255
#define KEEP_LOOPING 1

#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE  2116488
#endif

#ifndef GET_MEMORY
#define GET_MEMORY malloc
#endif

#ifndef P( mutex )
#define P( mutex )                                                          \
  do { int rc ;                                                             \
    if( ( rc = pthread_mutex_lock( &mutex ) ) != 0 )                        \
      ;/*TODO:log error */  \
  } while (0)
#endif  
        
#ifndef V( mutex )
#define V( mutex )                                                          \
        do { int rc ;                                                             \
            if( ( rc = pthread_mutex_unlock( &mutex ) ) != 0 )                      \
                ;/*TODO:log error */  \
  } while (0)
#endif

typedef struct stq_context {
    uint64_t len;
    char     context[MAX_CONTEXT_SIZE];
} stq_context_t;

typedef enum STQ_RETURN {
	STQ_SUCCESS=0,
	STQ_ERROR	
}STQ_RETURN_t;


/*  operation arguments */
typedef struct _op_item__ {
    /* operation info */
    void *arg;
    void  (*func)( );
    /* for chained list */
    struct op_item__ *p_next;

} stq_op_item_t;

typedef struct stq_queue__ {
    /* the queue */
    stq_op_item_t *first;
    stq_op_item_t *last;


    /* number of operations pending */
    uint64_t nb_waiting;

    pthread_mutex_t queues_mutex;

    pthread_cond_t work_avail_condition;
    pthread_cond_t work_done_condition;


    /* status (used for work_done_condition) */
    enum
    { PREPARING, IDLE, WORKING, FINISH } status;

} stq_queue_t;


/* thread info */
typedef struct stq_thread_info__ {
    pthread_t thread_id;
    uint64_t thr_index;
    uint64_t work_queue_count;
    stq_queue_t work_queue;
    stq_context_t consumer_specific_context;
    /* this pool is accessed by submitter
     * and by the consumer thread */
    pthread_mutex_t pool_mutex;
    pthread_cond_t req_condvar;
    void* stq_memory_op_pool;
} stq_thread_info_t;


/*all information of consumer  thread */
/*Function to submit the job in queue to be processed by thread  */
int stq_submit_to_Queue ( stq_thread_info_t *stq_thread ,
                          void (* func)() , void *p1 ,int s1, int worker_id );
/*Function to Init the thread
@arg1  -- thread info varibale for thread
@arg2  --  Number of work queue for thread
*/
void stq_thread_Init (stq_thread_info_t *stq_thread , int work_queue_count,
                      int thread_index );
#endif
