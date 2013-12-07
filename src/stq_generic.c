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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "stq_generic.h"
#include "log_helper.h"


#define THREAD_NAME_LEN 256
#ifndef NB_MAX_STQ_THREAD
#define NB_MAX_STQ_THREAD 1
#endif


extern stq_thread_info_t stq_slave_threads[NB_MAX_STQ_THREAD];
extern __thread thread_index;
/**
 *
 * stq_free_mem: free memory for stq task item.
 *
 * free memory for stq task item
 *
 * @param to_be_done [IN] stq task item to be freed .
 *
 * @return void.
 *
 */

void
stq_free_mem ( stq_op_item_t *to_be_done ){
    if(to_be_done) {
        if(to_be_done->arg)
            free(to_be_done->arg);
        free(to_be_done);
    }
}

/**
 *
 * stq_mgmt_thread: stq thread consuming it's work queue.
 *
 * STQ thread consuming it's work queue.
 *
 * @param arg [IN] stq_thread_info_t consisting thread resources  .
 *
 * @return void.
 *
 */

void *
stq_mgmt_thread(void *arg){

    stq_thread_info_t *p_info = (stq_thread_info_t *) arg;
    char thread_name[THREAD_NAME_LEN];
    stq_op_item_t *to_be_done;
    int rc = 0;
    static int i = 0;
    int worker_thread_id=0;
    thread_index = p_info->thr_index;
    /* initialize logging */
    snprintf(thread_name, 256, "stq_thread.%d #%u",i, p_info->thr_index);
    i++;
    SetNameFunction(thread_name);

#if 1
    int policy = SCHED_RR;

    struct sched_param param;
    param.sched_priority = 90;
    int s = pthread_setschedparam(pthread_self(), policy, &param);
    if (s != 0)
        STQ_LOG(LOG_INFO, "@@ Error seeting insert THREAD priority");
#endif
    /* main loop */
    while(KEEP_LOOPING) {
        P(p_info->work_queue.queues_mutex);
in_lock:
        to_be_done = NULL;
        if(p_info->work_queue.first != NULL) {
            /* take the next item in the list */
            to_be_done = p_info->work_queue.first;
            p_info->work_queue.first = to_be_done->p_next;

            /* still any entries in the list ? */
            if(p_info->work_queue.first == NULL)
                p_info->work_queue.last = NULL;
            /* it it the last entry ? */
            else if(p_info->work_queue.first->p_next == NULL)
                p_info->work_queue.last = p_info->work_queue.first;

            /* something to do */
            p_info->work_queue.status = WORKING;
            p_info->work_queue.nb_waiting--;

            V(p_info->work_queue.queues_mutex);

            /* PROCESS THE REQUEST */
            STQ_LOG(LOG_INFO, " processing queue request ...." );
            /*STQ function is supposed to reconstruct required arguements from packed struct
              to_be_done->arg */
            (*(to_be_done->func))(&(p_info->consumer_specific_context),
                    to_be_done->arg);
            stq_free_mem(to_be_done);
        } else {    
        /*CHECK*/ //V(p_info->work_queue.queues_mutex);
            pthread_cond_wait(&(p_info->req_condvar), &(p_info->work_queue.queues_mutex));
            goto in_lock;
        }

    }

}
/**
 *
 * init_thread_info: Intializes stq thread resources.
 *
 * Intializes stq thread resources.
 *
 * @param p_mthr_info [IN] stq_thread_info_t consisting thread resources  .
 * @param thread_index [IN] current thread index .
 * @param work_queue_count [IN] stq thread queue count.
 *
 * @return success or error .
 *
 */

int
init_thread_info(stq_thread_info_t * p_mthr_info,
        int thread_index ,int work_queue_count){

    unsigned int i;
    int worker_thread_id=0;
    if(!p_mthr_info)
        return 1;

    memset(p_mthr_info, 0, sizeof(stq_thread_info_t));

    p_mthr_info->thr_index  = thread_index ;
    p_mthr_info->work_queue_count = work_queue_count ;

    p_mthr_info->work_queue.first      = NULL;
    p_mthr_info->work_queue.last       = NULL;

    p_mthr_info->work_queue.nb_waiting = 0;

    if(pthread_mutex_init(&p_mthr_info->work_queue.queues_mutex, NULL))
        return 1;
    p_mthr_info->work_queue.status = PREPARING;


    if(pthread_mutex_init(&p_mthr_info->pool_mutex, NULL))
        return 1;

    if(pthread_cond_init(&p_mthr_info->req_condvar, NULL) != 0)
        return -1;

    return 0;


}
/**
 *
 * stq_thread_Init: Function to start stq threads .
 *
 * Function to start stq threads.
 *
 * @param stq_thread [IN] stq_thread_info_t consisting thread resources  .
 * @param work_queue_count [IN] stq thread queue count.
 * @param thread_index [IN] current thread index .
 *
 * @return success or error .
 *
 */

void
stq_thread_Init( stq_thread_info_t *stq_thread ,int work_queue_count ,
        int thread_index){
    int rc = 0;
    pthread_attr_t attr_thr;
    unsigned long i = 0 ;

    /* Init for thread parameter (mostly for scheduling) */
    pthread_attr_init(&attr_thr);
#ifndef _IRIX_6
    pthread_attr_setscope(&attr_thr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr_thr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr_thr, THREAD_STACK_SIZE);
#endif

    rc = init_thread_info(stq_thread, thread_index , work_queue_count);
    if(rc) {
        STQ_LOG(LOG_INFO,"error in initializing STQ thread %d " , rc);

        exit(1);
    }

    if((rc =  pthread_create(&(stq_thread->thread_id), &attr_thr, stq_mgmt_thread,
                    (void *)&(stq_thread->thread_id))) != 0) {
        STQ_LOG(LOG_INFO, "Error %d ", rc);

        exit(1);
    } else {
        STQ_LOG(LOG_INFO,"STQ thread  # started\n");
        pthread_detach(stq_thread->thread_id);
    }

}
/**
 *
 * stq_submit_to_Queue: Function to submit resources  stq queue .
 *
 * Function to submit resources  stq queue .
 *
 * @param stq_thread [IN] stq_thread_info_t consisting thread resources  .
 * @param func [IN] stq op function.
 * @param p1 [IN] stq resource.
 * @param s1 [IN] stq resource size.
 * @param worker_id [IN] worker thread id.
 *
 * @return success or error .
 *
 */


int
stq_submit_to_Queue( stq_thread_info_t *stq_thread ,
        void (* func)() , void *p1 ,int s1 , int worker_id){
    unsigned int i;
    stq_op_item_t *new_task;
    int rc = 0;

	
/*TODO add additional implementation for  
  scheduling algorithems
  ,priority setting 
  and user define metrics settings. */
  
    new_task = GET_MEMORY( sizeof(stq_op_item_t) );
    if(!new_task) {
        STQ_LOG(LOG_INFO,"error in allocating memory for mtime task queue ");
        return 1;
    }
    memset(new_task , 0 , sizeof(new_task));
    new_task->arg = NULL;
    /* arg inclused desired args passed as packed struct*/
    /* Consumer should know the packed struct details  */
    if(p1) {
        new_task->arg  = GET_MEMORY(s1);
        if(!new_task->arg) {
            STQ_LOG(LOG_INFO,"error in allocating memory for"
                    " stq task queue ");
            goto end;
        }

        memcpy(new_task->arg , p1 , s1);
    }

    new_task->func  = func;

    rc = stq_populate_q(stq_thread, &stq_thread->work_queue, new_task);

    return rc;
end:
    stq_free_mem(new_task);
    return 1;

}
/**
 *
 * insert_Queue: Inserts stq task to Queue .
 *
 * Inserts stq task to Queue .
 *
 * @param stq_thread [IN] stq_thread_info_t consisting thread resources  .
 * @param p_queue [IN] stq queue.
 * @param p_op [IN] stq operation resource.
 *
 * @return success or error .
 *
 */

int
stq_populate_q(stq_thread_info_t *stq_thread, stq_queue_t * p_queue, stq_op_item_t * p_op){
    P((p_queue->queues_mutex));
    /* add an item at the end of the queue */
    p_op->p_next = NULL;
    static uint64_t i = 0;
    if(p_queue->last == NULL) {
        /* first operation */
        p_queue->first = p_op;
        p_queue->last = p_op;
        i  = 1 ;
        if(pthread_cond_signal(&(stq_thread->req_condvar)) == -1) {
            STQ_LOG(LOG_INFO, "STQ Signal failed for  errno = %d",errno);
        }

    } else {
        i++;
        p_queue->last->p_next = p_op;
        p_queue->last = p_op;
    }

    STQ_LOG(LOG_INFO, "adding an item no %d to queue \n", i );
    p_queue->nb_waiting++;

   V((p_queue->queues_mutex));

    return 0;

}
/**
 *
 * stq_wait_thread_jobs_finished: Waits if there no job for stq .
 *
 * Waits if there no job for stq
 *
 * @param stq_thread [IN] stq_thread_info_t consisting thread resources  .
 * @param worker_id [IN] stq worker id.
 *
 * @return void .
 *
 */

void
stq_wait_thread_jobs_finished(stq_thread_info_t * p_thr_info ,int worker_id ){

    P(p_thr_info->work_queue.queues_mutex);
    while(p_thr_info->work_queue.first != NULL
            || p_thr_info->work_queue.status == WORKING)
        pthread_cond_wait(&p_thr_info->work_queue.work_done_condition,
                &p_thr_info->work_queue.queues_mutex);

   V(p_thr_info->work_queue.queues_mutex);

}



