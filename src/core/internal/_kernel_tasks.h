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

#ifndef __KERNEL_TASKS_H
#define __KERNEL_TASKS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "_kernel_sys.h"
#include "kernel_tasks.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Get pointer to `struct #KERNEL_Task` by pointer to the `task_queue` member
 * of the `struct #KERNEL_Task`.
 */
#define _kernel_get_task_by_tsk_queue(que)                                   \
   (que ? container_of(que, struct KERNEL_Task, task_queue) : 0)




/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

//-- functions for each task state: set, clear, check {{{

//-- runnable {{{

/**
 * Bring task to the $(KERNEL_TASK_STATE_RUNNABLE) state.
 * Should be called when task_state is NONE.
 *
 * Set RUNNABLE bit in task_state,
 * put task on the 'ready queue' for its priority,
 *
 * if priority of given `task` is higher than priority of
 * `_kernel_next_task_to_run`, then set `_kernel_next_task_to_run` to given `task`.
 */
void _kernel_task_set_runnable(struct KERNEL_Task *task);

/**
 * Bring task out from the $(KERNEL_TASK_STATE_RUNNABLE) state.
 * Should be called when task_state has just single RUNNABLE bit set.
 *
 * Clear RUNNABLE bit, remove task from 'ready queue', determine and set
 * new `#_kernel_next_task_to_run`.
 */
void _kernel_task_clear_runnable(struct KERNEL_Task *task);

/**
 * Returns whether given task is in $(KERNEL_TASK_STATE_RUNNABLE) state.
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_task_is_runnable(struct KERNEL_Task *task)
{
   return !!(task->task_state & KERNEL_TASK_STATE_RUNNABLE);
}

//}}}

//-- wait {{{

/**
 * Bring task to the $(KERNEL_TASK_STATE_WAIT) state.
 * Should be called when task_state is either NONE or $(KERNEL_TASK_STATE_SUSPEND).
 *
 * @param task
 *    Task to bring to the $(KERNEL_TASK_STATE_WAIT) state
 *
 * @param wait_que
 *    Wait queue to put task in, may be `#KERNEL_NULL`. If not `#KERNEL_NULL`, task is
 *    included in that list by `task_queue` member of `struct #KERNEL_Task`.
 *
 * @param wait_reason
 *    Reason of waiting, see `enum #KERNEL_WaitReason`.
 *
 * @param timeout
 *    If neither `0` nor `#KERNEL_WAIT_INFINITE`, task will be woken up by timer
 *    after specified number of system ticks.
 */
void _kernel_task_set_waiting(
      struct KERNEL_Task      *task,
      struct KERNEL_ListItem  *wait_que,
      enum KERNEL_WaitReason   wait_reason,
      KERNEL_TickCnt           timeout
      );

/**
 * Bring task out from the $(KERNEL_TASK_STATE_WAIT) state.
 * Task must be already in the $(KERNEL_TASK_STATE_WAIT) state. It may additionally
 * be in the $(KERNEL_TASK_STATE_SUSPEND) state.
 *
 * @param task
 *    Task to bring out from the $(KERNEL_TASK_STATE_WAIT) state
 *
 * @param wait_rc
 *    return code that will be returned to waiting task from waited function.
 */
void _kernel_task_clear_waiting(struct KERNEL_Task *task, enum KERNEL_RCode wait_rc);

/**
 * Returns whether given task is in $(KERNEL_TASK_STATE_WAIT) state.
 * Note that this state could be combined with $(KERNEL_TASK_STATE_SUSPEND) state.
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_task_is_waiting(struct KERNEL_Task *task)
{
   return !!(task->task_state & KERNEL_TASK_STATE_WAIT);
}

//}}}

//-- suspended {{{

/**
 * Bring task to the $(KERNEL_TASK_STATE_SUSPEND ) state.
 * Should be called when `task_state` is either NONE or $(KERNEL_TASK_STATE_WAIT).
 *
 * @param task
 *    Task to bring to the $(KERNEL_TASK_STATE_SUSPEND) state
 */
void _kernel_task_set_suspended(struct KERNEL_Task *task);

/**
 * Bring task out from the $(KERNEL_TASK_STATE_SUSPEND) state.
 * Task must be already in the $(KERNEL_TASK_STATE_SUSPEND) state. It may
 * additionally be in the $(KERNEL_TASK_STATE_WAIT) state.
 *
 * @param task
 *    Task to bring out from the $(KERNEL_TASK_STATE_SUSPEND) state
 */
void _kernel_task_clear_suspended(struct KERNEL_Task *task);

/**
 * Returns whether given task is in $(KERNEL_TASK_STATE_SUSPEND) state.
 * Note that this state could be combined with $(KERNEL_TASK_STATE_WAIT) state.
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_task_is_suspended(struct KERNEL_Task *task)
{
   return !!(task->task_state & KERNEL_TASK_STATE_SUSPEND);
}

//}}}

//-- dormant {{{

/**
 * Bring task to the $(KERNEL_TASK_STATE_DORMANT) state.
 * Should be called when task_state is NONE.
 *
 * Set DORMANT bit in task_state, reset task's priority to base value,
 * reset time slice count to 0.
 */
void _kernel_task_set_dormant(struct KERNEL_Task* task);

/**
 * Bring task out from the $(KERNEL_TASK_STATE_DORMANT) state.
 * Should be called when task_state has just single DORMANT bit set.
 *
 * Note: task's stack will be initialized inside this function (that is,
 * `#_kernel_arch_stack_init()` will be called)
 */
void _kernel_task_clear_dormant(struct KERNEL_Task *task);

/**
 * Returns whether given task is in $(KERNEL_TASK_STATE_DORMANT) state.
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_task_is_dormant(struct KERNEL_Task *task)
{
   return !!(task->task_state & KERNEL_TASK_STATE_DORMANT);
}

//}}}

//}}}



/**
 * Callback that is given to `_kernel_task_first_wait_complete()`, may perform
 * any needed actions before waking task up, e.g. set some data in the `struct
 * #KERNEL_Task` that task is waiting for.
 *
 * @param task
 *    Task that is going to be waken up
 *
 * @param user_data_1
 *    Arbitrary user data given to `_kernel_task_first_wait_complete()`
 *
 * @param user_data_2
 *    Arbitrary user data given to `_kernel_task_first_wait_complete()`
 */
typedef void (_KERNEL_CBBeforeTaskWaitComplete)(
      struct KERNEL_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      );

/**
 * See the comment for kernel_task_activate, kernel_task_iactivate in the kernel_tasks.h.
 *
 * It merely brings task out from the $(KERNEL_TASK_STATE_DORMANT) state and
 * brings it to the $(KERNEL_TASK_STATE_RUNNABLE) state.
 *
 * If task is not in the `DORMANT` state, `#KERNEL_RC_WSTATE` is returned.
 */
enum KERNEL_RCode _kernel_task_activate(struct KERNEL_Task *task);


/**
 * Should be called when task finishes waiting for anything.
 *
 * @param wait_rc return code that will be returned to waiting task
 */
_KERNEL_STATIC_INLINE void _kernel_task_wait_complete(struct KERNEL_Task *task, enum KERNEL_RCode wait_rc)
{
   _kernel_task_clear_waiting(task, wait_rc);

   //-- if task isn't suspended, make it runnable
   if (!_kernel_task_is_suspended(task)){
      _kernel_task_set_runnable(task);
   }

}


/**
 * Should be called when task starts waiting for anything.
 *
 * It merely calls `#_kernel_task_clear_runnable()` and then
 * `#_kernel_task_set_waiting()` for current task (`#_kernel_curr_run_task`).
 */
_KERNEL_STATIC_INLINE void _kernel_task_curr_to_wait_action(
      struct KERNEL_ListItem *wait_que,
      enum KERNEL_WaitReason wait_reason,
      KERNEL_TickCnt timeout
      )
{
   _kernel_task_clear_runnable(_kernel_curr_run_task);
   _kernel_task_set_waiting(_kernel_curr_run_task, wait_que, wait_reason, timeout);
}


/**
 * Change priority of any task (either runnable or non-runnable)
 */
void _kernel_change_task_priority(struct KERNEL_Task *task, int new_priority);

/**
 * When changing priority of the runnable task, this function
 * should be called instead of plain assignment.
 *
 * For non-runnable tasks, this function should never be called.
 *
 * Remove current task from ready queue for its current priority,
 * change its priority, add to the end of ready queue of new priority,
 * find next task to run.
 */
void  _kernel_change_running_task_priority(struct KERNEL_Task *task, int new_priority);

#if 0
#define _kernel_task_set_last_rc(rc)  { _kernel_curr_run_task = (rc); }

/**
 * If given return code is not `#KERNEL_RC_OK`, save it in the task's structure
 */
void _kernel_task_set_last_rc_if_error(enum KERNEL_RCode rc);
#endif

#if KERNEL_USE_MUTEXES
/**
 * Check if mutex is locked by task.
 *
 * @return KERNEL_TRUE if mutex is locked, KERNEL_FALSE otherwise.
 */
KERNEL_BOOL _kernel_is_mutex_locked_by_task(struct KERNEL_Task *task, struct KERNEL_Mutex *mutex);
#endif

/**
 * Wakes up first (if any) task from the queue, calling provided
 * callback before.
 *
 * @param wait_queue
 *    Wait queue to get first task from
 *
 * @param wait_rc
 *    Code that will be returned to woken-up task as a result of waiting
 *    (this code is just given to `_kernel_task_wait_complete()` actually)
 *
 * @param callback
 *    Callback function to call before wake task up, see
 *    `#_KERNEL_CBBeforeTaskWaitComplete`. Can be `KERNEL_NULL`.
 *
 * @param user_data_1
 *    Arbitrary data that is passed to the callback
 *
 * @param user_data_2
 *    Arbitrary data that is passed to the callback
 *
 *
 * @return
 *    - `KERNEL_TRUE` if queue is not empty and task has woken up
 *    - `KERNEL_FALSE` if queue is empty, so, no task to wake up
 */
KERNEL_BOOL _kernel_task_first_wait_complete(
      struct KERNEL_ListItem           *wait_queue,
      enum KERNEL_RCode                 wait_rc,
      _KERNEL_CBBeforeTaskWaitComplete *callback,
      void                         *user_data_1,
      void                         *user_data_2
      );


/**
 * The same as `kernel_task_exit(0)`, we need this function that takes no arguments
 * for exiting from task body function: we just set up initial task's stack so
 * that return address is `#_kernel_task_exit_nodelete`, and it works.
 *
 * If the function takes arguments, it becomes much harder.
 */
void _kernel_task_exit_nodelete(void);

/**
 * Returns end address of the stack. It depends on architecture stack
 * implementation, so there are two possible variants:
 *
 * - descending stack: `task->stack_low_addr`
 * - ascending stack:  `task->stack_high_addr`
 *
 * @param task
 *    Task in the subject
 *
 * @return
 *    End address of the stack.
 */
KERNEL_UWord *_kernel_task_stack_end_get(
      struct KERNEL_Task *task
      );

/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given task object is valid
 * (actually, just checks against `id_task` field, see `enum #KERNEL_ObjId`)
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_task_is_valid(
      const struct KERNEL_Task   *task
      )
{
   return (task->id_task == KERNEL_ID_TASK);
}



#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __KERNEL_TASKS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


