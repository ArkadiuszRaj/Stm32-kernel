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
 * \section kernel_tasks__tasks Task
 *
 * In KERNEL, a task is a branch of code that runs concurrently with other
 * tasks from the programmer's point of view. Indeed, tasks are actually
 * executed using processor time sharing.  Each task can be considered to be an
 * independed program, which executes in its own context (processor registers,
 * stack pointer, etc.).
 *
 * Actually, the term <i>thread</i> is more accurate than <i>task</i>, but the
 * term <i>task</i> historically was used in KERNELKernel, so KERNEL keeps this
 * convention.
 *
 * When kernel decides that it's time to run another task, it performs
 * <i>context switch</i>: current context (at least, values of all registers)
 * gets saved to the preempted task's stack, pointer to currently running
 * task is altered as well as stack pointer, and context gets restored from
 * the stack of newly running task.
 *
 * \section kernel_tasks__states Task states
 *
 * For list of task states and their description, refer to `enum
 * #KERNEL_TaskState`.
 *
 *
 * \section kernel_tasks__creating Creating/starting tasks
 *
 * Create task and start task are two separate actions; although you can
 * perform both of them in one step by passing `#KERNEL_TASK_CREATE_OPT_START` flag
 * to the `kernel_task_create()` function.
 *
 * \section kernel_tasks__stopping Stopping/deleting tasks
 *
 * Stop task and delete task are two separate actions. If task was just stopped
 * but not deleted, it can be just restarted again by calling
 * `kernel_task_activate()`. If task was deleted, it can't be just activated: it
 * should be re-created by `kernel_task_create()` first.
 *
 * Task stops execution when:
 *
 * - it calls `kernel_task_exit()`;
 * - it returns from its task body function (it is the equivalent to
 *   `kernel_task_exit(0)`)
 * - some other task calls `kernel_task_terminate()` passing appropriate pointer to
 *   `struct #KERNEL_Task`.
 *
 * \section kernel_tasks__scheduling Scheduling rules
 *
 * KERNEL always runs the most privileged task in state
 * $(KERNEL_TASK_STATE_RUNNABLE). In no circumstances can task run while there is
 * at least one task is in the $(KERNEL_TASK_STATE_RUNNABLE) state with higher
 * priority. Task will run until:
 *
 * - It becomes non-runnable (say, it may wait for something, etc)
 * - Some other task with higher priority becomes runnable.
 *
 * Tasks with the same priority may be scheduled in round robin fashion by
 * getting a predetermined time slice for each task with this priority.
 * Time slice is set separately for each priority. By default, round robin
 * is turned off for all priorities.
 *
 * \section kernel_tasks__idle Idle task
 *
 * KERNEL has one system task: an idle task, which has lowest priority.
 * It is always in the state $(KERNEL_TASK_STATE_RUNNABLE), and it runs only when
 * there are no other runnable tasks.
 *
 * User can provide a callback function to be called from idle task, see
 * #KERNEL_CBIdle. It is useful to bring the processor to some kind of real idle
 * state, so that device draws less current.
 *
 */

#ifndef _KERNEL_TASKS_H
#define _KERNEL_TASKS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_sys.h"
#include "kernel_list.h"
#include "kernel_common.h"

#include "kernel_eventgrp.h"
#include "kernel_dqueue.h"
#include "kernel_fmem.h"
#include "kernel_timer.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Task state
 */
enum KERNEL_TaskState {
   ///
   /// This state should never be publicly available.
   /// It may be stored in task_state only temporarily,
   /// while some system service is in progress.
   KERNEL_TASK_STATE_NONE         = 0,
   ///
   /// Task is ready to run (it doesn't mean that it is running at the moment)
   KERNEL_TASK_STATE_RUNNABLE     = (1 << 0),
   ///
   /// Task is waiting. The reason of waiting can be obtained from
   /// `task_wait_reason` field of the `struct KERNEL_Task`.
   ///
   /// @see `enum #KERNEL_WaitReason`
   KERNEL_TASK_STATE_WAIT         = (1 << 1),
   ///
   /// Task is suspended (by some other task)
   KERNEL_TASK_STATE_SUSPEND      = (1 << 2),
   ///
   /// Task was previously waiting, and after this it was suspended
   KERNEL_TASK_STATE_WAITSUSP     = (KERNEL_TASK_STATE_WAIT | KERNEL_TASK_STATE_SUSPEND),
   ///
   /// Task isn't yet activated or it was terminated by `kernel_task_terminate()`.
   KERNEL_TASK_STATE_DORMANT      = (1 << 3),


};


/**
 * Task wait reason
 */
enum KERNEL_WaitReason {
   ///
   /// Task isn't waiting for anything
   KERNEL_WAIT_REASON_NONE,
   ///
   /// Task has called `kernel_task_sleep()`
   KERNEL_WAIT_REASON_SLEEP,
   ///
   /// Task waits to acquire a semaphore
   /// @see kernel_sem.h
   KERNEL_WAIT_REASON_SEM,
   ///
   /// Task waits for some event in the event group to be set
   /// @see kernel_eventgrp.h
   KERNEL_WAIT_REASON_EVENT,
   ///
   /// Task wants to put some data to the data queue, and there's no space
   /// in the queue.
   /// @see kernel_dqueue.h
   KERNEL_WAIT_REASON_DQUE_WSEND,
   ///
   /// Task wants to receive some data to the data queue, and there's no data
   /// in the queue
   /// @see kernel_dqueue.h
   KERNEL_WAIT_REASON_DQUE_WRECEIVE,
   ///
   /// Task wants to lock a mutex with priority ceiling
   /// @see kernel_mutex.h
   KERNEL_WAIT_REASON_MUTEX_C,
   ///
   /// Task wants to lock a mutex with priority inheritance
   /// @see kernel_mutex.h
   KERNEL_WAIT_REASON_MUTEX_I,
   ///
   /// Task wants to get memory block from memory pool, and there's no free
   /// memory blocks
   /// @see kernel_fmem.h
   KERNEL_WAIT_REASON_WFIXMEM,


   ///
   /// Wait reasons count
   KERNEL_WAIT_REASONS_CNT
};

/**
 * Options for `kernel_task_create()`
 */
enum KERNEL_TaskCreateOpt {
   ///
   /// whether task should be activated right after it is created.
   /// If this flag is not set, user must activate task manually by calling
   /// `kernel_task_activate()`.
   KERNEL_TASK_CREATE_OPT_START = (1 << 0),
   ///
   /// for internal kernel usage only: this option must be provided
   /// when creating idle task
   _KERNEL_TASK_CREATE_OPT_IDLE = (1 << 1),
};

/**
 * Options for `kernel_task_exit()`
 */
enum KERNEL_TaskExitOpt {
   ///
   /// whether task should be deleted right after it is exited.
   /// If this flag is not set, user must either delete it manually by
   /// calling `kernel_task_delete()` or re-activate it by calling
   /// `kernel_task_activate()`.
   KERNEL_TASK_EXIT_OPT_DELETE = (1 << 0),
};

#if KERNEL_PROFILER || DOXYGEN_ACTIVE
/**
 * Timing structure that is managed by profiler and can be read by
 * `#kernel_task_profiler_timing_get()` function. This structure is contained in
 * each `struct #KERNEL_Task` structure.
 *
 * Available if only `#KERNEL_PROFILER` option is non-zero, also depends on
 * `#KERNEL_PROFILER_WAIT_TIME`.
 */
struct KERNEL_TaskTiming {
   ///
   /// Total time when task was running.
   ///
   /// \attention
   /// This is NOT the time that task was in $(KERNEL_TASK_STATE_RUNNABLE) state:
   /// if task A is preempted by high-priority task B, task A is not running,
   /// but is still in the $(KERNEL_TASK_STATE_RUNNABLE) state. This counter
   /// represents the time task was actually <b>running</b>.
   unsigned long long   total_run_time;
   ///
   /// How many times task got running. It is useful to find an average
   /// value of consecutive running time: `(total_run_time / got_running_cnt)`
   unsigned long long   got_running_cnt;
   ///
   /// Maximum consecutive time task was running.
   unsigned long        max_consecutive_run_time;

#if KERNEL_PROFILER_WAIT_TIME || DOXYGEN_ACTIVE
   ///
   /// Available if only `#KERNEL_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Total time when task was not running; time is broken down by reasons of
   /// waiting.
   ///
   /// For example, to get the time task was waiting for mutexes with priority
   /// inheritance protocol, use: `total_wait_time[ #KERNEL_WAIT_REASON_MUTEX_I ]`
   ///
   /// To get the time task was runnable but preempted by another task, use:
   /// `total_wait_time[ #KERNEL_WAIT_REASON_NONE ]`
   ///
   unsigned long long   total_wait_time[ KERNEL_WAIT_REASONS_CNT ];
   ///
   /// Available if only `#KERNEL_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Maximum consecutive time task was not running; time is broken down by
   /// reasons of waiting.
   ///
   /// @see `total_wait_time`
   unsigned long        max_consecutive_wait_time[ KERNEL_WAIT_REASONS_CNT ];
#endif
};

/**
 * Internal kernel structure for profiling data of task.
 *
 * Available if only `#KERNEL_PROFILER` option is non-zero.
 */
struct _KERNEL_TaskProfiler {
   ///
   /// Tick count of when the task got running or non-running last time.
   KERNEL_TickCnt        last_tick_cnt;
#if KERNEL_PROFILER_WAIT_TIME || DOXYGEN_ACTIVE
   ///
   /// Available if only `#KERNEL_PROFILER_WAIT_TIME` option is non-zero.
   ///
   /// Value of `task->task_wait_reason` when task got non-running last time.
   enum KERNEL_WaitReason   last_wait_reason;
#endif

#if KERNEL_DEBUG
   ///
   /// For internal profiler self-check only: indicates whether task is
   /// running or not. Available if only `#KERNEL_DEBUG` is non-zero.
   int                  is_running;
#endif
   ///
   /// Main timing structure managed by profiler. Contents of this structure
   /// can be read by `#kernel_task_profiler_timing_get()` function.
   struct KERNEL_TaskTiming timing;
};
#endif

/**
 * Task
 */
struct KERNEL_Task {
   /// pointer to task's current top of the stack;
   /// Note that this field **must** be a first field in the struct,
   /// this fact is exploited by platform-specific routines.
   KERNEL_UWord *stack_cur_pt;
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   /// For `struct KERNEL_Task`, we can't make it the very first field, since
   /// stack pointer should be there.
   enum KERNEL_ObjId id_task;
   ///
   /// queue is used to include task in ready/wait lists
   struct KERNEL_ListItem task_queue;
   ///
   /// timer object to implement task waiting for timeout
   struct KERNEL_Timer timer;
   ///
   /// pointer to object's (semaphore, mutex, event, etc) wait list in which
   /// task is included for waiting
   struct KERNEL_ListItem *pwait_queue;
   ///
   /// queue is used to include task in creation list
   /// (currently, this list is used for statistics only)
   struct KERNEL_ListItem create_queue;

#if KERNEL_USE_MUTEXES
   ///
   /// list of all mutexes that are locked by task
   struct KERNEL_ListItem mutex_queue;
#if KERNEL_MUTEX_DEADLOCK_DETECT
   ///
   /// list of other tasks involved in deadlock. This list is non-empty
   /// only in emergency cases, and it is here to help you fix your bug
   /// that led to deadlock.
   ///
   /// @see `#KERNEL_MUTEX_DEADLOCK_DETECT`
   struct KERNEL_ListItem deadlock_list;
#endif
#endif

   ///-- lowest address of stack. It is independent of architecture:
   ///   it's always the lowest address (which may be actually origin
   ///   or end of stack, depending on the architecture)
   KERNEL_UWord *stack_low_addr;
   ///-- Highest address of stack. It is independent of architecture:
   ///   it's always the highest address (which may be actually origin
   ///   or end of stack, depending on the architecture)
   KERNEL_UWord *stack_high_addr;
   ///
   /// pointer to task's body function given to `kernel_task_create()`
   KERNEL_TaskBody *task_func_addr;
   ///
   /// pointer to task's parameter given to `kernel_task_create()`
   void *task_func_param;
   ///
   /// base priority of the task (actual current priority may be higher than
   /// base priority because of mutex)
   int base_priority;
   ///
   /// current task priority
   int priority;
   ///
   /// task state
   enum KERNEL_TaskState task_state;
   ///
   /// reason for waiting (relevant if only `task_state` is
   /// $(KERNEL_TASK_STATE_WAIT) or $(KERNEL_TASK_STATE_WAITSUSP))
   enum KERNEL_WaitReason task_wait_reason;
   ///
   /// waiting result code (reason why waiting finished)
   enum KERNEL_RCode task_wait_rc;
   //
   // remaining time until timeout; may be `#KERNEL_WAIT_INFINITE`.
   //KERNEL_TickCnt tick_count;
   ///
   /// time slice counter
   int tslice_count;
#if 0
   ///
   /// last operation result code, might be used if some service
   /// does not return that code directly
   int last_rc;
#endif
   ///
   /// subsystem-specific fields that are used while task waits for something.
   /// Do note that these fields are grouped by union, so, they must not
   /// interfere with each other. It's quite ok here because task can't wait
   /// for different things.
   union {
      /// fields specific to kernel_eventgrp.h
      struct KERNEL_EGrpTaskWait eventgrp;
      ///
      /// fields specific to kernel_dqueue.h
      struct KERNEL_DQueueTaskWait dqueue;
      ///
      /// fields specific to kernel_fmem.h
      struct KERNEL_FMemTaskWait fmem;
   } subsys_wait;
   ///
   /// Task name for debug purposes, user may want to set it by hand
   const char *name;
#if KERNEL_PROFILER || DOXYGEN_ACTIVE
   /// Profiler data, available if only `#KERNEL_PROFILER` is non-zero.
   struct _KERNEL_TaskProfiler    profiler;
#endif

   /// Internal flag used to optimize mutex priority algorithms.
   /// For the comments on it, see file kernel_mutex.c,
   /// function `_mutex_do_unlock()`.
   unsigned          priority_already_updated : 1;

   /// Flag indicates that task waited for something
   /// This flag is set automatially in `_kernel_task_set_waiting()`
   /// Must be cleared manually before calling any service that could sleep,
   /// if the caller is interested in the relevant value of this flag.
   unsigned          waited : 1;


// Other implementation specific fields may be added below

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
 * Construct task and probably start it (depends on options, see below).
 * `id_task` member should not contain `#KERNEL_ID_TASK`, otherwise,
 * `#KERNEL_RC_WPARAM` is returned.
 *
 * Usage example:
 *
 * \code{.c}
 *     #define MY_TASK_STACK_SIZE   (KERNEL_MIN_STACK_SIZE + 200)
 *     #define MY_TASK_PRIORITY     5
 *
 *     struct KERNEL_Task my_task;
 *
 *     //-- define stack array, we use convenience macro KERNEL_STACK_ARR_DEF()
 *     //   for that
 *     KERNEL_STACK_ARR_DEF(my_task_stack, MY_TASK_STACK_SIZE);
 *
 *     void my_task_body(void *param)
 *     {
 *        //-- an endless loop
 *        for (;;){
 *           kernel_task_sleep(1);
 *
 *           //-- probably do something useful
 *        }
 *     }
 * \endcode
 *
 *
 *
 * And then, somewhere from other task or from the callback
 * `#KERNEL_CBUserTaskCreate` given to `kernel_sys_start()` :
 * \code{.c}
 *    enum KERNEL_RCode rc = kernel_task_create(
 *          &my_task,
 *          my_task_body,
 *          MY_TASK_PRIORITY,
 *          my_task_stack,
 *          MY_TASK_STACK_SIZE,
 *          KERNEL_NULL,                     //-- parameter isn't used
 *          KERNEL_TASK_CREATE_OPT_START  //-- start task on creation
 *          );
 *
 *    if (rc != KERNEL_RC_OK){
 *       //-- handle error
 *    }
 * \endcode
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task
 *    Ready-allocated `struct KERNEL_Task` structure. `id_task` member should not
 *    contain `#KERNEL_ID_TASK`, otherwise `#KERNEL_RC_WPARAM` is returned.
 * @param task_func
 *    Pointer to task body function.
 * @param priority
 *    Priority for new task. **NOTE**: the lower value, the higher priority.
 *    Must be > `0` and < `(#KERNEL_PRIORITIES_CNT - 1)`.
 * @param task_stack_low_addr
 *    Pointer to the stack for task.
 *    User must either use the macro `KERNEL_STACK_ARR_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#KERNEL_UWord` with
 *    `#KERNEL_ARCH_STK_ATTR_BEFORE` and `#KERNEL_ARCH_STK_ATTR_AFTER` macros.
 * @param task_stack_size
 *    Size of task stack array, in words (`#KERNEL_UWord`), not in bytes.
 * @param param
 *    Parameter that is passed to `task_func`.
 * @param opts
 *    Options for task creation, refer to `enum #KERNEL_TaskCreateOpt`
 *
 * @return
 *    * `#KERNEL_RC_OK` on success;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_WPARAM` if wrong params were given;
 *
 * @see `#kernel_task_create_wname()`
 * @see `#KERNEL_ARCH_STK_ATTR_BEFORE`
 * @see `#KERNEL_ARCH_STK_ATTR_AFTER`
 */
enum KERNEL_RCode kernel_task_create(
      struct KERNEL_Task         *task,
      KERNEL_TaskBody            *task_func,
      int                     priority,
      KERNEL_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum KERNEL_TaskCreateOpt   opts
      );


/**
 * The same as `kernel_task_create()` but with additional argument `name`,
 * which could be very useful for debug.
 */
enum KERNEL_RCode kernel_task_create_wname(
      struct KERNEL_Task         *task,
      KERNEL_TaskBody            *task_func,
      int                     priority,
      KERNEL_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum KERNEL_TaskCreateOpt   opts,
      const char             *name
      );

/**
 * If the task is $(KERNEL_TASK_STATE_RUNNABLE), it is moved to the
 * $(KERNEL_TASK_STATE_SUSPEND) state. If the task is in the $(KERNEL_TASK_STATE_WAIT)
 * state, it is moved to the $(KERNEL_TASK_STATE_WAITSUSP) state.  (waiting +
 * suspended)
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    Task to suspend
 *
 * @return
 *    * `#KERNEL_RC_OK` on success;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_WSTATE` if task is already suspended or dormant;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see `enum #KERNEL_TaskState`
 */
enum KERNEL_RCode kernel_task_suspend(struct KERNEL_Task *task);

/**
 * Release task from $(KERNEL_TASK_STATE_SUSPEND) state. If the given task is in
 * the $(KERNEL_TASK_STATE_SUSPEND) state, it is moved to $(KERNEL_TASK_STATE_RUNNABLE)
 * state; afterwards it has the lowest precedence among runnable tasks with the
 * same priority. If the task is in $(KERNEL_TASK_STATE_WAITSUSP) state, it is
 * moved to $(KERNEL_TASK_STATE_WAIT) state.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    Task to release from suspended state
 *
 * @return
 *    * `#KERNEL_RC_OK` on success;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_WSTATE` if task is not suspended;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see enum KERNEL_TaskState
 */
enum KERNEL_RCode kernel_task_resume(struct KERNEL_Task *task);

/**
 * Put current task to sleep for at most timeout ticks. When the timeout
 * expires and the task was not suspended during the sleep, it is switched to
 * runnable state. If the timeout value is `#KERNEL_WAIT_INFINITE` and the task was
 * not suspended during the sleep, the task will sleep until another function
 * call (like `kernel_task_wakeup()` or similar) will make it runnable.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param timeout
 *    Refer to `#KERNEL_TickCnt`
 *
 * @returns
 *    * `#KERNEL_RC_TIMEOUT` if task has slept specified timeout;
 *    * `#KERNEL_RC_OK` if task was woken up from other task by `kernel_task_wakeup()`
 *    * `#KERNEL_RC_FORCED` if task was released from wait forcibly by
 *       `kernel_task_release_wait()`
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context
 *
 * @see KERNEL_TickCnt
 */
enum KERNEL_RCode kernel_task_sleep(KERNEL_TickCnt timeout);

/**
 * Wake up task from sleep.
 *
 * Task is woken up if only it sleeps because of call to `kernel_task_sleep()`.
 * If task sleeps for some another reason, task won't be woken up,
 * and `kernel_task_wakeup()` returns `#KERNEL_RC_WSTATE`.
 *
 * After this call, `kernel_task_sleep()` returns `#KERNEL_RC_OK`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    sleeping task to wake up
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WSTATE` if task is not sleeping, or it is sleeping for
 *       some reason other than `kernel_task_sleep()` call.
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 */
enum KERNEL_RCode kernel_task_wakeup(struct KERNEL_Task *task);

/**
 * The same as `kernel_task_wakeup()` but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_task_iwakeup(struct KERNEL_Task *task);

/**
 * Activate task that is in $(KERNEL_TASK_STATE_DORMANT) state, that is, it was
 * either just created by `kernel_task_create()` without
 * `#KERNEL_TASK_CREATE_OPT_START` option, or terminated.
 *
 * Task is moved from $(KERNEL_TASK_STATE_DORMANT) state to the
 * $(KERNEL_TASK_STATE_RUNNABLE) state.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    dormant task to activate
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WSTATE` if task is not dormant
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 * @see KERNEL_TaskState
 */
enum KERNEL_RCode kernel_task_activate(struct KERNEL_Task *task);

/**
 * The same as `kernel_task_activate()` but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_task_iactivate(struct KERNEL_Task *task);

/**
 * Release task from $(KERNEL_TASK_STATE_WAIT) state, independently of the reason
 * of waiting.
 *
 * If task is in $(KERNEL_TASK_STATE_WAIT) state, it is moved to
 * $(KERNEL_TASK_STATE_RUNNABLE) state.  If task is in $(KERNEL_TASK_STATE_WAITSUSP)
 * state, it is moved to $(KERNEL_TASK_STATE_SUSPEND) state.
 *
 * `#KERNEL_RC_FORCED` is returned to the waiting task.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * \attention Usage of this function is discouraged, since the need for
 * it indicates bad software design
 *
 * @param task    task waiting for anything
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WSTATE` if task is not waiting for anything
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 *
 * @see KERNEL_TaskState
 */
enum KERNEL_RCode kernel_task_release_wait(struct KERNEL_Task *task);

/**
 * The same as `kernel_task_release_wait()` but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_task_irelease_wait(struct KERNEL_Task *task);

/**
 * This function terminates the currently running task. The task is moved to
 * the $(KERNEL_TASK_STATE_DORMANT) state.
 *
 * After exiting, the task may be either deleted by the `kernel_task_delete()`
 * function call or reactivated by the `kernel_task_activate()` /
 * `kernel_task_iactivate()` function call. In this case task starts execution from
 * beginning (as after creation/activation).  The task will have the lowest
 * precedence among all tasks with the same priority in the
 * $(KERNEL_TASK_STATE_RUNNABLE) state.
 *
 * If this function is invoked with `#KERNEL_TASK_EXIT_OPT_DELETE` option set, the
 * task will be deleted after termination and cannot be reactivated (needs
 * recreation).
 *
 * Please note that returning from task body function has the same effect as
 * calling `kernel_task_exit(0)`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return
 *    Returns if only called from wrong context. Normally, it never returns
 *    (since calling task becomes terminated)
 *
 * @see `#KERNEL_TASK_EXIT_OPT_DELETE`
 * @see `kernel_task_delete()`
 * @see `kernel_task_activate()`
 * @see `kernel_task_iactivate()`
 */
void kernel_task_exit(enum KERNEL_TaskExitOpt opts);


/**
 * This function is similar to `kernel_task_exit()` but it terminates any task
 * other than currently running one.
 *
 * After task is terminated, the task may be either deleted by the
 * `kernel_task_delete()` function call or reactivated by the `kernel_task_activate()`
 * / `kernel_task_iactivate()` function call. In this case task starts execution
 * from beginning (as after creation/activation).  The task will have the
 * lowest precedence among all tasks with the same priority in the
 * $(KERNEL_TASK_STATE_RUNNABLE) state.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    task to terminate
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WSTATE` if task is already dormant
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_task_terminate(struct KERNEL_Task *task);

/**
 * This function deletes the task specified by the task. The task must be in
 * the $(KERNEL_TASK_STATE_DORMANT) state, otherwise `#KERNEL_RC_WCONTEXT` will be
 * returned.
 *
 * This function resets the `id_task` field in the task structure to 0 and
 * removes the task from the system tasks list. The task can not be reactivated
 * after this function call (the task must be recreated).
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task    dormant task to delete
 *
 * @return
 *    * `#KERNEL_RC_OK` if successful
 *    * `#KERNEL_RC_WSTATE` if task is not dormant
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 */
enum KERNEL_RCode kernel_task_delete(struct KERNEL_Task *task);

/**
 * Get current state of the task; note that returned state is a bitmask,
 * that is, states could be combined with each other.
 *
 * Currently, only $(KERNEL_TASK_STATE_WAIT) and $(KERNEL_TASK_STATE_SUSPEND) states
 * are allowed to be set together. Nevertheless, it would be probably good
 * idea to test individual bits in the returned value instead of plain
 * comparing values.
 *
 * Note that if something goes wrong, variable pointed to by `p_state`
 * isn't touched.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task
 *    task to get state of
 * @param p_state
 *    pointer to the location where to store state of the task
 *
 * @return state of the task
 */
enum KERNEL_RCode kernel_task_state_get(
      struct KERNEL_Task *task,
      enum KERNEL_TaskState *p_state
      );

#if KERNEL_PROFILER || DOXYGEN_ACTIVE
/**
 * Read profiler timing data of the task. See `struct #KERNEL_TaskTiming` for
 * details on timing data.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param task
 *    Task to get timing data of
 * @param tgt
 *    Target structure to fill with data, should be allocated by caller
 */
enum KERNEL_RCode kernel_task_profiler_timing_get(
      const struct KERNEL_Task *task,
      struct KERNEL_TaskTiming *tgt
      );
#endif


/**
 * Set new priority for task.
 * If priority is 0, then task's base_priority is set.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_LEGEND_LINK)
 *
 * \attention this function is obsolete and will probably be removed
 */
enum KERNEL_RCode kernel_task_change_priority(struct KERNEL_Task *task, int new_priority);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_TASKS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


