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
 * A mutex is an object used to protect shared resources.
 *
 * There is a lot of confusion about the differences between semaphores and
 * mutexes, so, it's highly recommended that you read a small article by
 * Michael Barr: [Mutexes and Semaphores Demystified](http://goo.gl/YprPBW).
 *
 * Very short:
 *
 * While a mutex is seemingly similar to a semaphore with a maximum count of `1`
 * (the so-called binary semaphore), their usage is very different: the purpose
 * of mutex is to protect a shared resource. A locked mutex is "owned" by the
 * task that locked it, and only that same task may unlock it. This ownership
 * allows you to implement algorithms to prevent priority inversion.  So,
 * a mutex is a *locking mechanism*.
 *
 * A semaphore, on the other hand, is a *signaling mechanism*. It's quite legal
 * and encouraged for a semaphore to be acquired in task A, and then signaled
 * from task B or even from an ISR. It may be used in situations like "producer
 * and consumer", etc.
 *
 * In addition to the article mentioned above, you may want to look at the
 * [related question on stackoverflow.com](http://goo.gl/ZBReHK).
 *
 * ---------------------------------------------------------------------------
 *
 * Mutex features in KERNEL:
 *
 *    - Recursive locking is supported (if option `#KERNEL_MUTEX_REC` is non-zero);
 *    - Deadlock detection (if option `#KERNEL_MUTEX_DEADLOCK_DETECT` is non-zero);
 *    - Two protocols available to avoid unbounded priority inversion: priority
 *      inheritance and priority ceiling.
 *
 *
 * A discussion about the strengths and weaknesses of each protocol as
 * well as the priority inversions problem is beyond the scope of this document.
 *
 * The priority inheritance protocol solves the priority inversion problem, but
 * doesn't prevent deadlocks. However, the kernel can notify you if a deadlock
 * has occurred (see `#KERNEL_MUTEX_DEADLOCK_DETECT`).
 *
 * The priority ceiling protocol prevents deadlocks and chained blocking but it
 * is slower than the priority inheritance protocol.
 *
 * @see `#KERNEL_USE_MUTEXES`
 */

#ifndef _KERNEL_MUTEX_H
#define _KERNEL_MUTEX_H

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
 * Mutex protocol for avoid priority inversion
 */
enum KERNEL_MutexProtocol {
   ///
   /// Mutex uses priority ceiling protocol
   KERNEL_MUTEX_PROT_CEILING = 1,
   ///
   /// Mutex uses priority inheritance protocol
   KERNEL_MUTEX_PROT_INHERIT = 2,
};


/**
 * Mutex
 */
struct KERNEL_Mutex {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum KERNEL_ObjId id_mutex;
   ///
   /// List of tasks that wait a mutex
   struct KERNEL_ListItem wait_queue;
   ///
   /// To include in task's locked mutexes list (if any)
   struct KERNEL_ListItem mutex_queue;
#if KERNEL_MUTEX_DEADLOCK_DETECT
   ///
   /// List of other mutexes involved in deadlock
   /// (normally, this list is empty)
   struct KERNEL_ListItem deadlock_list;
#endif
   ///
   /// Mutex protocol: priority ceiling or priority inheritance
   enum KERNEL_MutexProtocol protocol;
   ///
   /// Current mutex owner (task that locked mutex)
   struct KERNEL_Task *holder;
   ///
   /// Used if only protocol is `#KERNEL_MUTEX_PROT_CEILING`:
   /// maximum priority of task that may lock the mutex
   int ceil_priority;
   ///
   /// Lock count (for recursive locking)
   int cnt;
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
 * Construct the mutex. The field `id_mutex` should not contain `#KERNEL_ID_MUTEX`,
 * otherwise, `#KERNEL_RC_WPARAM` is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param mutex
 *    Pointer to already allocated `struct KERNEL_Mutex`
 * @param protocol
 *    Mutex protocol: priority ceiling or priority inheritance.
 *    See `enum #KERNEL_MutexProtocol`.
 * @param ceil_priority
 *    Used if only `protocol` is `#KERNEL_MUTEX_PROT_CEILING`: maximum priority
 *    of the task that may lock the mutex.
 *
 * @return
 *    * `#KERNEL_RC_OK` if mutex was successfully created;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#KERNEL_RC_WPARAM`.
 */
enum KERNEL_RCode kernel_mutex_create(
      struct KERNEL_Mutex        *mutex,
      enum KERNEL_MutexProtocol   protocol,
      int                     ceil_priority
      );

/**
 * Destruct mutex.
 *
 * All tasks that wait for lock the mutex become runnable with
 * `#KERNEL_RC_DELETED` code returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param mutex      mutex to destruct
 *
 * @return
 *    * `#KERNEL_RC_OK` if mutex was successfully destroyed;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_mutex_delete(struct KERNEL_Mutex *mutex);

/**
 * Lock mutex.
 *
 *    * If the mutex is not locked, function immediately locks the mutex and
 *      returns `#KERNEL_RC_OK`.
 *    * If the mutex is already locked by the same task, lock count is merely
 *      incremented and `#KERNEL_RC_OK` is returned immediately.
 *    * If the mutex is locked by different task, behavior depends on
 *      `timeout` value: refer to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param mutex      mutex to lock
 * @param timeout    refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK` if mutex is successfully locked or if lock count was
 *      merely incremented (this is possible if recursive locking is enabled,
 *      see `#KERNEL_MUTEX_REC`)
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_ILLEGAL_USE`
 *       * if mutex protocol is `#KERNEL_MUTEX_PROT_CEILING`
 *         and calling task's priority is higher than `ceil_priority`
 *         given to `kernel_mutex_create()`
 *       * if recursive locking is disabled (see `#KERNEL_MUTEX_REC`)
 *         and the mutex is already locked by calling task
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see `#KERNEL_MutexProtocol`
 */
enum KERNEL_RCode kernel_mutex_lock(struct KERNEL_Mutex *mutex, KERNEL_TickCnt timeout);

/**
 * The same as `kernel_mutex_lock()` with zero timeout
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_mutex_lock_polling(struct KERNEL_Mutex *mutex);

/**
 * Unlock mutex.
 *    * If mutex is not locked or locked by different task, `#KERNEL_RC_ILLEGAL_USE`
 *      is returned.
 *    * If mutex is already locked by calling task, lock count is decremented.
 *      Now, if lock count is zero, mutex gets unlocked (and if there are
 *      task(s) waiting for mutex, the first one from the wait queue locks the
 *      mutex).  Otherwise, mutex remains locked with lock count decremented
 *      and function returns `#KERNEL_RC_OK`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return
 *    * `#KERNEL_RC_OK` if mutex is unlocked of if lock count was merely decremented
 *      (this is possible if recursive locking is enabled, see `#KERNEL_MUTEX_REC`)
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_ILLEGAL_USE` if mutex is either not locked or locked by
 *      different task
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 */
enum KERNEL_RCode kernel_mutex_unlock(struct KERNEL_Mutex *mutex);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_MUTEX_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


