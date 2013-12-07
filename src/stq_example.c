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
int nb_stq_threads=0;
#define SIZE_OF_DOER_DATA 32 /*bytes*/
/**
 *
 * start_stq_threads: Initializes  deferred process  threads  .
 *
 * Initializes  STQ threads
 *
 * @param [IN] number of STQ threads
 *
 * @return STQ_SUCCESS if eviction threads are started successfully, other values show an error.
 *
 */

STQ_RETURN_t
start_stq_threads(int no_of_stq_threads){
    int index =0;
    nb_stq_threads=no_of_stq_threads;
    for(; index<no_of_stq_threads; index++) {
        /*There is one queue per stq thread*/
        stq_thread_Init(&stq_slave_threads[index],1,index);
    }
    return STQ_SUCCESS;
}
/**
 *
 * populate_tasks_to_stq: Submits  tasks to stq queues .
 *
 * 
 * Caller should submit this function to stq submit
 *
 * @param void *in-data
 *
 * @return _SUCCESS if process is success, other values show an error.
 *
 */

STQ_RETURN_t
populate_tasks_to_stq(void *p_in_data){
    STQ_RETURN_t ret = STQ_SUCCESS;

    int slave_thread=0;
    int nb_taker_threads=nb_stq_threads;
    void *p_data_to_passed_to_doer=p_in_data;
    int size_of_doer_data=SIZE_OF_DOER_DATA;

    while(nb_taker_threads--) {
        if(stq_submit_to_Queue(&stq_slave_threads[slave_thread++],
                    backend_doer_function,(void *) p_data_to_passed_to_doer , size_of_doer_data,
                    slave_thread)) {
            STQ_LOG(LOG_INFO,"error in submitting to the dc eviction queue");
            ret=STQ_ERROR;
            goto end;
        }
        STQ_LOG(LOG_INFO, "submitted to stq queue %d ",slave_thread);
    }
end:
    return ret;
}


/**
 *
 * backend_doer_function: Consumes ops from stq queues  .
 *
 * Consumes ops from stq queues
 * 
 *
 * @param p_stq_context [IN] stq context .
 * @param arg [IN] args of void *
 *
 * @return STQ_SUCCESS if operation is success, other values show an error.
 *
 */

void
backend_doer_function(stq_context_t *p_stq_context,void *arg){
    /*USE context and data to operate backend functions*/

end:
    return;
}

