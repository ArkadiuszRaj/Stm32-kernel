/*******************************************************************************
 *
 * KERNEL: real-time kernel initially based on KERNELKernel
 *
 *    KERNELKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    KERNEL:                      copyright 2014       Dmitry Frank.
 *
 *    KERNEL was born as a thorough review and re-implementation of
 *    KERNELKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with KERNELKernel,
 *    hence the new name: KERNEL.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FIKERNELESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * \file
 *
 * A data queue is a FIFO that stores pointer (of type `void *`) in each cell,
 * called (in uITRON style) a data element. A data queue also has an associated
 * wait queue each for sending (`wait_send` queue) and for receiving
 * (`wait_receive` queue).  A task that sends a data element tries to put the
 * data element into the FIFO. If there is no space left in the FIFO, the task
 * is switched to the waiting state and placed in the data queue's `wait_send`
 * queue until space appears (another task gets a data element from the data
 * queue).
 *
 * A task that receives a data element tries to get a data element
 * from the FIFO. If the FIFO is empty (there is no data in the data queue),
 * the task is switched to the waiting state and placed in the data queue's
 * `wait_receive` queue until data element arrive (another task puts some data
 * element into the data queue).  To use a data queue just for the synchronous
 * message passing, set size of the FIFO to 0.  The data element to be sent and
 * received can be interpreted as a pointer or an integer and may have value 0
 * (`KERNEL_NULL`).
 *
 * For the useful pattern on how to use queue together with \ref kernel_fmem.h
 * "fixed memory pool", refer to the example: `examples/queue`. Be sure
 * to examine the readme there.
 *
 * KERNEL offers a way to wait for a message from multiple queues in just a
 * single call, refer to the section \ref eventgrp_connect for details. Related
 * queue services:
 *
 * - `kernel_queue_eventgrp_connect()`
 * - `kernel_queue_eventgrp_disconnect()`
 *
 * There is an example project available that demonstrates event group
 * connection technique: `examples/queue_eventgrp_conn`. Be sure to examine the
 * readme there.
 *
 */

#ifndef _KERNEL_DQUEUE_H
#define _KERNEL_DQUEUE_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_list.h"
#include "kernel_common.h"
#include "kernel_eventgrp.h"



/*******************************************************************************
 *    EXTERN TYPES
 ******************************************************************************/



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Structure representing data queue object
 */
struct KERNEL_DQueue {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum KERNEL_ObjId id_dque;
   ///
   /// list of tasks waiting to send data
   struct KERNEL_ListItem  wait_send_list;
   ///
   /// list of tasks waiting to receive data
   struct KERNEL_ListItem  wait_receive_list;

   ///
   /// array of `void *` to store data queue items. Can be `KERNEL_NULL`.
   void         **data_fifo;
   ///
   /// capacity (total items count). Can be 0.
   int            items_cnt;
   ///
   /// count of non-free items in `data_fifo`
   int            filled_items_cnt;
   ///
   /// index of the item which will be written next time
   int            head_idx;
   ///
   /// index of the item which will be read next time
   int            tail_idx;
   ///
   /// connected event group
   struct KERNEL_EGrpLink eventgrp_link;
};

/**
 * DQueue-specific fields related to waiting task,
 * to be included in struct KERNEL_Task.
 */
struct KERNEL_DQueueTaskWait {
   /// if task tries to send the data to the data queue,
   /// and there's no space in the queue, value to put to queue is stored
   /// in this field
   void *data_elem;
};


/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct data queue. `id_dque` member should not contain `#KERNEL_ID_DATAQUEUE`,
 * otherwise, `#KERNEL_RC_WPARAM` is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque       pointer to already allocated struct KERNEL_DQueue.
 * @param data_fifo  pointer to already allocated array of `void *` to store
 *                   data queue items. Can be `#KERNEL_NULL`.
 * @param items_cnt  capacity of queue
 *                   (count of elements in the `data_fifo` array)
 *                   Can be 0.
 *
 * @return
 *    * `#KERNEL_RC_OK` if queue was successfully created;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#KERNEL_RC_WPARAM`.
 */
enum KERNEL_RCode kernel_queue_create(
      struct KERNEL_DQueue *dque,
      void **data_fifo,
      int items_cnt
      );


/**
 * Destruct data queue.
 *
 * All tasks that wait for writing to or reading from the queue become
 * runnable with `#KERNEL_RC_DELETED` code returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque       pointer to data queue to be deleted
 *
 * @return
 *    * `#KERNEL_RC_OK` if queue was successfully deleted;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_queue_delete(struct KERNEL_DQueue *dque);


/**
 * Send the data element specified by the `p_data` to the data queue
 * specified by the `dque`.
 *
 * If there are tasks in the data queue's `wait_receive` list already, the
 * function releases the task from the head of the `wait_receive` list, makes
 * this task runnable and transfers the parameter `p_data` to task's
 * function, that caused it to wait.
 *
 * If there are no tasks in the data queue's `wait_receive` list, parameter
 * `p_data` is placed to the tail of data FIFO. If the data FIFO is full,
 * behavior depends on the `timeout` value: refer to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque       pointer to data queue to send data to
 * @param p_data     value to send
 * @param timeout    refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK`   if data was successfully sent;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see `#KERNEL_TickCnt`
 */
enum KERNEL_RCode kernel_queue_send(
      struct KERNEL_DQueue *dque,
      void *p_data,
      KERNEL_TickCnt timeout
      );

/**
 * The same as `kernel_queue_send()` with zero timeout
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_send_polling(
      struct KERNEL_DQueue *dque,
      void *p_data
      );

/**
 * The same as `kernel_queue_send()` with zero timeout, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_isend_polling(
      struct KERNEL_DQueue *dque,
      void *p_data
      );

/**
 * Receive the data element from the data queue specified by the `dque` and
 * place it into the address specified by the `pp_data`.  If the FIFO already
 * has data, function removes an entry from the end of the data queue FIFO and
 * returns it into the `pp_data` function parameter.
 *
 * If there are task(s) in the data queue's `wait_send` list, first one gets
 * removed from the head of `wait_send` list, becomes runnable and puts the
 * data entry, stored in this task, to the tail of data FIFO.  If there are no
 * entries in the data FIFO and there are no tasks in the wait_send list,
 * behavior depends on the `timeout` value: refer to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque       pointer to data queue to receive data from
 * @param pp_data    pointer to location to store the value
 * @param timeout    refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK`   if data was successfully received;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see `#KERNEL_TickCnt`
 */
enum KERNEL_RCode kernel_queue_receive(
      struct KERNEL_DQueue *dque,
      void **pp_data,
      KERNEL_TickCnt timeout
      );

/**
 * The same as `kernel_queue_receive()` with zero timeout
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_receive_polling(
      struct KERNEL_DQueue *dque,
      void **pp_data
      );

/**
 * The same as `kernel_queue_receive()` with zero timeout, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_ireceive_polling(
      struct KERNEL_DQueue *dque,
      void **pp_data
      );


/**
 * Returns number of free items in the queue
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque
 *    Pointer to queue.
 *
 * @return
 *    Number of free items in the queue, or -1 if wrong params were given (the
 *    check is performed if only `#KERNEL_CHECK_PARAM` is non-zero)
 */
int kernel_queue_free_items_cnt_get(
      struct KERNEL_DQueue    *dque
      );


/**
 * Returns number of used (non-free) items in the queue
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param dque
 *    Pointer to queue.
 *
 * @return
 *    Number of used (non-free) items in the queue, or -1 if wrong params were
 *    given (the check is performed if only `#KERNEL_CHECK_PARAM` is non-zero)
 */
int kernel_queue_used_items_cnt_get(
      struct KERNEL_DQueue    *dque
      );


/**
 * Connect an event group to the queue.
 * Refer to the section \ref eventgrp_connect for details.
 *
 * Only one event group can be connected to the queue at a time. If you
 * connect event group while another event group is already connected,
 * the old link is discarded.
 *
 * @param dque
 *    queue to which event group should be connected
 * @param eventgrp
 *    event groupt to connect
 * @param pattern
 *    flags pattern that should be managed by the queue automatically
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_eventgrp_connect(
      struct KERNEL_DQueue    *dque,
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             pattern
      );


/**
 * Disconnect a connected event group from the queue.
 * Refer to the section \ref eventgrp_connect for details.
 *
 * If there is no event group connected, nothing is changed.
 *
 * @param dque    queue from which event group should be disconnected
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_queue_eventgrp_disconnect(
      struct KERNEL_DQueue    *dque
      );


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_DQUEUE_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


