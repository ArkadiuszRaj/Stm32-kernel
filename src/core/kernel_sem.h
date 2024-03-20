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
 * A semaphore: an object to provide signaling mechanism.
 *
 * There is a lot of confusion about differences between semaphores and
 * mutexes, so, it's quite recommended to read small article by Michael Barr:
 * [Mutexes and Semaphores Demystified](http://goo.gl/YprPBW).
 *
 * Very short:
 *
 * While mutex is seemingly similar to a semaphore with maximum count of `1`
 * (the so-called binary semaphore), their usage is very different: the purpose
 * of mutex is to protect shared resource. A locked mutex is "owned" by the
 * task that locked it, and only the same task may unlock it. This ownership
 * allows to implement algorithms to prevent priority inversion.  So, mutex is
 * a *locking mechanism*.
 *
 * Semaphore, on the other hand, is *signaling mechanism*. It's quite legal and
 * encouraged for semaphore to be waited for in the task A, and then signaled
 * from task B or even from ISR. It may be used in situations like "producer
 * and consumer", etc.
 *
 * In addition to the article mentioned above, you may want to look at the
 * [related question on stackoverflow.com](http://goo.gl/ZBReHK).
 *
 */

#ifndef _KERNEL_SEM_H
#define _KERNEL_SEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_list.h"
#include "kernel_common.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Semaphore
 */
struct KERNEL_Sem {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum KERNEL_ObjId id_sem;
   ///
   /// List of tasks that wait for the semaphore
   struct KERNEL_ListItem wait_queue;
   ///
   /// Current semaphore counter value
   int count;
   ///
   /// Max value of `count`
   int max_count;
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
 * Construct the semaphore. `id_sem` field should not contain
 * `#KERNEL_ID_SEMAPHORE`, otherwise, `#KERNEL_RC_WPARAM` is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param sem
 *    Pointer to already allocated `struct KERNEL_Sem`
 * @param start_count
 *    Initial counter value, typically it is equal to `max_count`
 * @param max_count
 *    Maximum counter value.
 *
 * @return
 *    * `#KERNEL_RC_OK` if semaphore was successfully created;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#KERNEL_RC_WPARAM`.
 */
enum KERNEL_RCode kernel_sem_create(
      struct KERNEL_Sem *sem,
      int start_count,
      int max_count
      );

/**
 * Destruct the semaphore.
 *
 * All tasks that wait for the semaphore become runnable with
 * `#KERNEL_RC_DELETED` code returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param sem     semaphore to destruct
 *
 * @return
 *    * `#KERNEL_RC_OK` if semaphore was successfully deleted;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_sem_delete(struct KERNEL_Sem *sem);

/**
 * Signal the semaphore.
 *
 * If current semaphore counter (`count`) is less than `max_count`, counter is
 * incremented by one, and first task (if any) that \ref kernel_sem_wait() "waits"
 * for the semaphore becomes runnable with `#KERNEL_RC_OK` returned from
 * `kernel_sem_wait()`.
 *
 * if semaphore counter is already has its max value, no action performed and
 * `#KERNEL_RC_OVERFLOW` is returned
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param sem     semaphore to signal
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_OVERFLOW` if `count` is already at maximum value (`max_count`)
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_sem_signal(struct KERNEL_Sem *sem);

/**
 * The same as `kernel_sem_signal()` but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 */
enum KERNEL_RCode kernel_sem_isignal(struct KERNEL_Sem *sem);

/**
 * Wait for the semaphore.
 *
 * If the current semaphore counter (`count`) is non-zero, it is decremented
 * and `#KERNEL_RC_OK` is returned. Otherwise, behavior depends on `timeout` value:
 * task might switch to $(KERNEL_TASK_STATE_WAIT) state until someone \ref
 * kernel_sem_signal "signaled" the semaphore or until the `timeout` expired. refer
 * to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param sem     semaphore to wait for
 * @param timeout refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK` if waiting was successfull
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_sem_wait(struct KERNEL_Sem *sem, KERNEL_TickCnt timeout);

/**
 * The same as `kernel_sem_wait()` with zero timeout.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_sem_wait_polling(struct KERNEL_Sem *sem);

/**
 * The same as `kernel_sem_wait()` with zero timeout, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_sem_iwait_polling(struct KERNEL_Sem *sem);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_SEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


