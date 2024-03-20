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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common kernelkernel headers
#include "kernel_common.h"
#include "kernel_sys.h"

//-- internal kernelkernel headers
#include "_kernel_tasks.h"
#include "_kernel_mutex.h"
#include "_kernel_timer.h"
#include "_kernel_list.h"


//-- header of current module
#include "_kernel_tasks.h"

//-- header of other needed modules
#include "kernel_mutex.h"

//-- std header for memset() and memcpy()
#include <string.h>



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if KERNEL_CHECK_PARAM
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_generic(
      const struct KERNEL_Task *task
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (task == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_task_is_valid(task)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

#else
#  define _check_param_generic(task)            (KERNEL_RC_OK)
#endif
// }}}

//-- Private utilities {{{

#if KERNEL_USE_MUTEXES

_KERNEL_STATIC_INLINE void _init_mutex_queue(struct KERNEL_Task *task)
{
   _kernel_list_reset(&(task->mutex_queue));
}

#if KERNEL_MUTEX_DEADLOCK_DETECT
_KERNEL_STATIC_INLINE void _init_deadlock_list(struct KERNEL_Task *task)
{
   _kernel_list_reset(&(task->deadlock_list));
}
#else
#  define   _init_deadlock_list(task)
#endif

#else
#  define   _init_mutex_queue(task)
#  define   _init_deadlock_list(task)
#endif


/**
 * Looks for first runnable task with highest priority,
 * set _kernel_next_task_to_run to it.
 *
 * @return `KERNEL_TRUE` if _kernel_next_task_to_run was changed, `KERNEL_FALSE` otherwise.
 */
static void _find_next_task_to_run(void)
{
   int priority;

#ifdef _KERNEL_FFS
   //-- architecture-dependent way to find-first-set-bit is available,
   //   so use it.
   priority = _KERNEL_FFS(_kernel_ready_to_run_bmp);
   priority--;
#else
   //-- there is no architecture-dependent way to find-first-set-bit available,
   //   so, use generic (somewhat naive) algorithm.
   int i;
   unsigned int mask;

   mask = 1;
   priority = 0;

   for (i = 0; i < KERNEL_PRIORITIES_CNT; i++){
      //-- for each bit in bmp
      if (_kernel_ready_to_run_bmp & mask){
         priority = i;
         break;
      }
      mask = (mask << 1);
   }
#endif

   //-- set task to run: fetch next task from ready list of appropriate
   //   priority.
   _kernel_next_task_to_run = _kernel_get_task_by_tsk_queue(
         _kernel_tasks_ready_list[priority].next
         );
}

// }}}

//-- Inline functions {{{


/**
 * See the comment for kernel_task_wakeup, kernel_task_iwakeup in the kernel_tasks.h
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _task_wakeup(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (     (_kernel_task_is_waiting(task))
         && (task->task_wait_reason == KERNEL_WAIT_REASON_SLEEP))
   {
      //-- Task is sleeping, so, let's wake it up.
      _kernel_task_wait_complete(task, KERNEL_RC_OK);

   } else {
      //-- Task isn't sleeping. Probably it is in WAIT state,
      //   but not because of call to kernel_task_sleep().

      rc = KERNEL_RC_WSTATE;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _task_release_wait(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if ((_kernel_task_is_waiting(task))){
      //-- task is in WAIT state, so, let's release it from that state,
      //   returning KERNEL_RC_FORCED.
      _kernel_task_wait_complete(task, KERNEL_RC_FORCED);
   } else {
      rc = KERNEL_RC_WSTATE;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _task_delete(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (!_kernel_task_is_dormant(task)){
      //-- Cannot delete not-terminated task
      rc = KERNEL_RC_WSTATE;
   } else {
      _kernel_list_remove_entry(&(task->create_queue));
      _kernel_tasks_created_cnt--;
      task->id_task = KERNEL_ID_NONE;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _task_job_perform(
      struct KERNEL_Task *task,
      enum KERNEL_RCode (p_worker)(struct KERNEL_Task *task)
      )
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      //-- proceed to real job
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      rc = p_worker(task);

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();
   }
   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _task_job_iperform(
      struct KERNEL_Task *task,
      enum KERNEL_RCode (p_worker)(struct KERNEL_Task *task)
      )
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();

      //-- call actual worker function (_task_wakeup, _kernel_task_activate, etc)
      rc = p_worker(task);

      KERNEL_INT_IRESTORE();
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();

   }
   return rc;
}

/**
 * Returns KERNEL_TRUE if there are no more items in the runqueue for given
 * priority, KERNEL_FALSE otherwise.
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _remove_entry_from_ready_queue(
      struct KERNEL_ListItem *list_node,
      int priority
      )
{
   KERNEL_BOOL ret;

   //-- remove given list_node from the queue
   _kernel_list_remove_entry(list_node);

   //-- check if the queue for given priority is empty now
   ret = _kernel_list_is_empty(&(_kernel_tasks_ready_list[priority]));

   if (ret){
      //-- list is empty, so, modify bitmask _kernel_ready_to_run_bmp
      _kernel_ready_to_run_bmp &= ~(1 << priority);
   }

   return ret;
}

_KERNEL_STATIC_INLINE void _add_entry_to_ready_queue(
      struct KERNEL_ListItem *list_node, int priority
      )
{
   _kernel_list_add_tail(&(_kernel_tasks_ready_list[priority]), list_node);
   _kernel_ready_to_run_bmp |= (1 << priority);
}

// }}}

/**
 * handle current wait_reason: say, for MUTEX_I, we should
 * handle priorities of other involved tasks.
 *
 * This function is called _after_ removing task from wait_queue,
 * because _find_max_blocked_priority()
 * in kernel_mutex.c checks for all tasks in mutex's wait_queue to
 * get max blocked priority.

 * This function is called _before_ task is actually woken up,
 * so callback functions may check whatever waiting parameters
 * task had, such as task_wait_reason and pwait_queue.
 *
 *   TODO: probably create callback list, so, when task_wait_reason
 *         is set to KERNEL_WAIT_REASON_MUTEX_I/.._C
 *         (this is done in kernel_mutex.c, _add_curr_task_to_mutex_wait_queue())
 *         it should add its callback that will be called
 *         when task stops waiting.
 *
 *         But it seems too much overhead: we need to allocate a memory for that
 *         every time callback is registered.
 */
static void _on_task_wait_complete(struct KERNEL_Task *task)
{
   //-- for mutex with priority inheritance, call special handler
   if (task->task_wait_reason == KERNEL_WAIT_REASON_MUTEX_I){
      _kernel_mutex_i_on_task_wait_complete(task);
   }

   //-- for any mutex, call special handler
   if (     (task->task_wait_reason == KERNEL_WAIT_REASON_MUTEX_I)
         || (task->task_wait_reason == KERNEL_WAIT_REASON_MUTEX_C)
      )
   {
      _kernel_mutex_on_task_wait_complete(task);
   }

}

/**
 * NOTE: task_state should be set to KERNEL_TASK_STATE_NONE before calling.
 *
 * Teminate task:
 *    * unlock all mutexes that are held by task
 *    * set dormant state (reinitialize everything)
 *    * reitinialize stack
 */
static void _task_terminate(struct KERNEL_Task *task)
{
#if KERNEL_DEBUG
   if (task->task_state != KERNEL_TASK_STATE_NONE){
      _KERNEL_FATAL_ERROR("");
   }
#endif

   //-- Unlock all mutexes locked by the task
   _kernel_mutex_unlock_all_by_task(task);

   //-- task is already in the state NONE, so, we just need
   //   to set dormant state.
   _kernel_task_set_dormant(task);
}

/**
 * This function is called by timer
 */
static void _task_wait_timeout(struct KERNEL_Timer *timer, void *p_user_data)
{
   struct KERNEL_Task *task = (struct KERNEL_Task *)p_user_data;

   //-- interrupts should be enabled
   //   NOTE: we can't perform this check on PIC24/dsPIC, because on this arch
   //   KERNEL has a distinction between "user" and "system" interrupts, and
   //   KERNEL_IS_INT_DISABLED() returns true if (IPL >= system_IPL_level).
   //   Since timer callback is called from system interrupt,
   //   KERNEL_IS_INT_DISABLED() returns true.
   //
   //   TODO: probably make a distinction between disabled interrupts and
   //   system IPL level.
#if !defined(__KERNEL_ARCH_PIC24_DSPIC__)
   _KERNEL_BUG_ON( KERNEL_IS_INT_DISABLED() );
#endif

   //-- since timer callback is called with interrupts enabled,
   //   we need to disable them before calling `_kernel_task_wait_complete()`.
   KERNEL_INTSAVE_DATA_INT;
   KERNEL_INT_IDIS_SAVE();

   _kernel_task_wait_complete(task, KERNEL_RC_TIMEOUT);

   KERNEL_INT_IRESTORE();

   _KERNEL_UNUSED(timer);
}

/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_create(
      struct KERNEL_Task         *task,
      KERNEL_TaskBody            *task_func,
      int                     priority,
      KERNEL_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum KERNEL_TaskCreateOpt   opts
      )
{
   KERNEL_INTSAVE_DATA;
   enum KERNEL_RCode rc;
   enum KERNEL_Context context;

   int i;

   //-- Lightweight checking of system tasks recreation
   if (     priority == (KERNEL_PRIORITIES_CNT - 1)
         && !(opts & _KERNEL_TASK_CREATE_OPT_IDLE)
      )
   {
      return KERNEL_RC_WPARAM;
   }

   if (0
         || (priority < 0 || priority > (KERNEL_PRIORITIES_CNT - 1))
         || task_stack_size < KERNEL_MIN_STACK_SIZE
         || task_func == KERNEL_NULL
         || task == KERNEL_NULL
         || task_stack_low_addr == KERNEL_NULL
         || _kernel_task_is_valid(task)
      )
   {
      return KERNEL_RC_WPARAM;
   }

   rc = KERNEL_RC_OK;

   context = kernel_sys_context_get();

   //-- Note: since `kernel_task_create()` is called from `kernel_sys_start()`, it
   //   is allowed to have `#KERNEL_CONTEXT_NONE` here. In this case,
   //   interrupts aren't disabled/enabled.
   if (context != KERNEL_CONTEXT_TASK && context != KERNEL_CONTEXT_NONE){
      return KERNEL_RC_WCONTEXT;
   }

   if (context == KERNEL_CONTEXT_TASK){
      KERNEL_INT_DIS_SAVE();
   }

   //--- Init task structure
   task->task_func_addr  = task_func;
   task->task_func_param = param;

   task->stack_low_addr = task_stack_low_addr;
   task->stack_high_addr = task_stack_low_addr + task_stack_size - 1;

   task->base_priority   = priority;
   task->task_state      = KERNEL_TASK_STATE_NONE;
   task->id_task         = KERNEL_ID_TASK;

   task->task_wait_reason = KERNEL_WAIT_REASON_NONE;
   task->task_wait_rc = KERNEL_RC_OK;

   task->pwait_queue  = KERNEL_NULL;

#if KERNEL_PROFILER
   memset(&task->profiler, 0x00, sizeof(task->profiler));
#endif

   //-- fill all task stack space by #KERNEL_FILL_STACK_VAL
   {
      KERNEL_UWord *ptr_stack;
      for (
            i = 0, ptr_stack = task_stack_low_addr;
            i < task_stack_size;
            i++
          )
      {
         *ptr_stack++ = KERNEL_FILL_STACK_VAL;
      }
   }

   //-- reset task_queue (the queue used to include task to runqueue or
   //   waitqueue)
   _kernel_list_reset(&(task->task_queue));

   //-- init timer that is needed to implement task wait timeout
   _kernel_timer_create(&task->timer, _task_wait_timeout, task);

   //-- init auxiliary lists needed for tasks
   _init_mutex_queue(task);
   _init_deadlock_list(task);

   //-- Set initial task state: `KERNEL_TASK_STATE_DORMANT`
   _kernel_task_set_dormant(task);

   //-- Add task to created task queue
   _kernel_list_add_tail(&_kernel_tasks_created_list, &(task->create_queue));
   _kernel_tasks_created_cnt++;

   if ((opts & KERNEL_TASK_CREATE_OPT_START)){
      _kernel_task_activate(task);
   }

   if (context == KERNEL_CONTEXT_TASK){
      KERNEL_INT_RESTORE();
      if ((opts & KERNEL_TASK_CREATE_OPT_START)){
         _kernel_context_switch_pend_if_needed();
      }
   }

   return rc;
}


enum KERNEL_RCode kernel_task_create_wname(
      struct KERNEL_Task         *task,
      KERNEL_TaskBody            *task_func,
      int                     priority,
      KERNEL_UWord               *task_stack_low_addr,
      int                     task_stack_size,
      void                   *param,
      enum KERNEL_TaskCreateOpt   opts,
      const char             *name
      )
{
   enum KERNEL_RCode ret = kernel_task_create(
         task, task_func, priority, task_stack_low_addr, task_stack_size,
         param, opts
         );

   //-- if task was successfully created, set the name
   if (ret == KERNEL_RC_OK){
      task->name = name;
   }

   return ret;
}



/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_suspend(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      if (_kernel_task_is_suspended(task) || _kernel_task_is_dormant(task)){
         //-- task is already suspended, or it is dormant;
         //   in either case, the state is wrong for suspending.
         rc = KERNEL_RC_WSTATE;
      } else {

         //-- if task is runnable, clear runnable state.
         //   Note: it might be waiting instead of runnable: then,
         //   don't do anything with 'waiting' state: it is legal
         //   for task to be in waiting + suspended state.
         //   (KERNEL_TASK_STATE_WAITSUSP)
         if (_kernel_task_is_runnable(task)){
            _kernel_task_clear_runnable(task);
         }

         //-- set suspended state
         _kernel_task_set_suspended(task);

      }

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();

   }
   return rc;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_resume(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      if (!_kernel_task_is_suspended(task)){
         //-- task isn't suspended; this is wrong state.
         rc = KERNEL_RC_WSTATE;
      } else {

         //-- clear suspended state
         _kernel_task_clear_suspended(task);

         if (!_kernel_task_is_waiting(task)){
            //-- The task is not in the WAIT-SUSPEND state,
            //   so we need to make it runnable and probably switch context
            _kernel_task_set_runnable(task);
         }

      }

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();

   }
   return rc;

}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_sleep(KERNEL_TickCnt timeout)
{
   enum KERNEL_RCode rc;

   if (timeout == 0){
      rc = KERNEL_RC_TIMEOUT;
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- put task to wait with reason SLEEP and without wait queue.
      _kernel_task_curr_to_wait_action(KERNEL_NULL, KERNEL_WAIT_REASON_SLEEP, timeout);

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();
      rc = _kernel_curr_run_task->task_wait_rc;

   }

   return rc;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_wakeup(struct KERNEL_Task *task)
{
   return _task_job_perform(task, _task_wakeup);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_iwakeup(struct KERNEL_Task *task)
{
   return _task_job_iperform(task, _task_wakeup);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_activate(struct KERNEL_Task *task)
{
   return _task_job_perform(task, _kernel_task_activate);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_iactivate(struct KERNEL_Task *task)
{
   return _task_job_iperform(task, _kernel_task_activate);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_release_wait(struct KERNEL_Task *task)
{
   return _task_job_perform(task, _task_release_wait);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_irelease_wait(struct KERNEL_Task *task)
{
   return _task_job_iperform(task, _task_release_wait);
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
void kernel_task_exit(enum KERNEL_TaskExitOpt opts)
{
   struct KERNEL_Task *task;

   if (!kernel_is_task_context()){
      //-- do nothing, just return
   } else {

      //-- here, we unconditionally disable interrupts:
      //   this function never returns, and interrupt status is restored
      //   from different task's stack inside
      //   `_kernel_arch_context_switch_now_nosave()` call.
      kernel_arch_int_dis();

      task = _kernel_curr_run_task;

      //-- clear runnable state of currently running task,
      //   and terminate it

      _kernel_task_clear_runnable(task);
      _task_terminate(task);

      if ((opts & KERNEL_TASK_EXIT_OPT_DELETE)){
         //-- after exiting from task, we should delete it as well
         //   (because appropriate flag was set)
         _task_delete(task);
      }

      //-- interrupts will be enabled inside _kernel_arch_context_switch_now_nosave()
      _kernel_arch_context_switch_now_nosave();
   }

   return;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_terminate(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      if (_kernel_task_is_dormant(task)){
         //-- The task is already terminated
         rc = KERNEL_RC_WSTATE;
      } else if (_kernel_curr_run_task == task){
         //-- Cannot terminate currently running task
         //   (use kernel_task_exit() instead)
         rc = KERNEL_RC_WCONTEXT;
      } else {

         if (_kernel_task_is_runnable(task)){
            //-- if task is runnable, we must clear runnable state
            //   before terminating the task
            _kernel_task_clear_runnable(task);
         } else if (_kernel_task_is_waiting(task)){
            //-- if task is waiting, we must clear waiting state
            //   before terminating the task.
            //   No matter what waiting result code we specify:
            //   nobody is going to read it.
            _kernel_task_clear_waiting(
                  task,
                  KERNEL_RC_OK    //-- doesn't matter: nobody will read it
                  );
         }

         if (_kernel_task_is_suspended(task)){
            //-- if task is suspended, we must clear suspended state
            //   before terminating the task
            _kernel_task_clear_suspended(task);
         }

         //-- eventually, terminate the task
         _task_terminate(task);
      }

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();

   }
   return rc;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_delete(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();
      rc = _task_delete(task);
      KERNEL_INT_RESTORE();

   }
   return rc;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_state_get(
      struct KERNEL_Task *task,
      enum KERNEL_TaskState *p_state
      )
{
   enum KERNEL_RCode rc = _check_param_generic(task);
   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();
      *p_state = task->task_state;
      KERNEL_INT_RESTORE();
   }
   return rc;
}

/*
 * See comments in the header file (kernel_tasks.h)
 */
enum KERNEL_RCode kernel_task_change_priority(struct KERNEL_Task *task, int new_priority)
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (new_priority < 0 || new_priority >= (KERNEL_PRIORITIES_CNT - 1)){
      rc = KERNEL_RC_WPARAM;
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      if (new_priority == 0){
         new_priority = task->base_priority;
      }

      rc = KERNEL_RC_OK;

      if (_kernel_task_is_dormant(task)){
         rc = KERNEL_RC_WSTATE;
      } else {
         _kernel_change_task_priority(task, new_priority);
      }

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();
   }
   return rc;
}

#if KERNEL_PROFILER
enum KERNEL_RCode kernel_task_profiler_timing_get(
      const struct KERNEL_Task *task,
      struct KERNEL_TaskTiming *tgt
      )
{
   enum KERNEL_RCode rc = _check_param_generic(task);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {
      int sr_saved;
      sr_saved = kernel_arch_sr_save_int_dis();

      //-- just copy timing data from task structure
      //   to the user-provided location
      memcpy(tgt, &task->profiler.timing, sizeof(*tgt));

      kernel_arch_sr_restore(sr_saved);
   }
   return rc;
}
#endif




/*******************************************************************************
 *    INTERNAL KERNELKERNEL FUNCTIONS
 ******************************************************************************/

/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_task_set_runnable(struct KERNEL_Task * task)
{
#if KERNEL_DEBUG
   //-- task_state should be NONE here
   if (task->task_state != KERNEL_TASK_STATE_NONE){
      _KERNEL_FATAL_ERROR("");
   }
#endif

   int priority;

   priority          = task->priority;
   task->task_state  |= KERNEL_TASK_STATE_RUNNABLE;

   //-- Add the task to the end of 'ready queue' for the current priority
   _add_entry_to_ready_queue(&(task->task_queue), priority);

   //-- less value - greater priority, so '<' operation is used here
   if (priority < _kernel_next_task_to_run->priority){
      _kernel_next_task_to_run = task;
   }
}

/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_task_clear_runnable(struct KERNEL_Task *task)
{
#if KERNEL_DEBUG
   //-- task_state should be exactly KERNEL_TASK_STATE_RUNNABLE here
   if (task->task_state != KERNEL_TASK_STATE_RUNNABLE){
      _KERNEL_FATAL_ERROR("");
   }

   if (task == &_kernel_idle_task){
      //-- idle task should always be runnable
      _KERNEL_FATAL_ERROR("idle task should always be runnable");
   }
#endif

   //KERNEL_BOOL ret = KERNEL_FALSE;

   int priority;
   priority = task->priority;

   //-- remove runnable state
   task->task_state &= ~KERNEL_TASK_STATE_RUNNABLE;

   //-- remove the curr task from any queue (now - from ready queue)
   if (_remove_entry_from_ready_queue(&(task->task_queue), priority)){
      //-- No ready tasks for the curr priority

      //-- Find highest priority ready to run -
      //   at least, lowest-priority bit must be set for the idle task
      //   (the exact bit position actually depends on KERNEL_PRIORITIES_CNT value)
      _find_next_task_to_run();

   } else {
      //-- There are 'ready to run' task(s) for the curr priority

      if (_kernel_next_task_to_run == task){
         //-- the task that just became non-runnable was the "next task to run",
         //   so we should select new next task to run
         _kernel_next_task_to_run = _kernel_get_task_by_tsk_queue(
               _kernel_tasks_ready_list[priority].next
               );

         //-- _kernel_next_task_to_run was just altered, so, we should return KERNEL_TRUE
      }
   }

   //-- and reset task's queue
   _kernel_list_reset(&(task->task_queue));

}

void _kernel_task_set_waiting(
      struct KERNEL_Task *task,
      struct KERNEL_ListItem *wait_que,
      enum KERNEL_WaitReason wait_reason,
      KERNEL_TickCnt timeout
      )
{
#if KERNEL_DEBUG
   //-- only SUSPEND bit is allowed here
   if (task->task_state & ~(KERNEL_TASK_STATE_SUSPEND)){
      _KERNEL_FATAL_ERROR("");
   } else if (timeout == 0){
      _KERNEL_FATAL_ERROR("");
   } else if (_kernel_timer_is_active(&task->timer)){
      _KERNEL_FATAL_ERROR("");
   }

#endif

   task->task_state       |= KERNEL_TASK_STATE_WAIT;
   task->task_wait_reason = wait_reason;

   task->waited           = KERNEL_TRUE;

   //--- Add to the wait queue  - FIFO

   if (wait_que != KERNEL_NULL){
      _kernel_list_add_tail(wait_que, &(task->task_queue));
      task->pwait_queue = wait_que;
   } else {
      //-- NOTE: we don't need to reset task_queue because
      //   it is already reset in _kernel_task_clear_runnable().
   }

   //-- Add to the timers queue, if timeout is neither 0 nor `KERNEL_WAIT_INFINITE`.
   _kernel_timer_start(&task->timer, timeout);
}

/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_task_clear_waiting(struct KERNEL_Task *task, enum KERNEL_RCode wait_rc)
{
#if KERNEL_DEBUG
   //-- only WAIT and SUSPEND bits are allowed here,
   //   and WAIT bit must be set
   if (
              (task->task_state & ~(KERNEL_TASK_STATE_WAIT | KERNEL_TASK_STATE_SUSPEND))
         || (!(task->task_state &  (KERNEL_TASK_STATE_WAIT)                    ))
      )
   {
      _KERNEL_FATAL_ERROR("");
   }

   if (_kernel_list_is_empty(&task->task_queue) != (task->pwait_queue == KERNEL_NULL)){
      _KERNEL_FATAL_ERROR("task_queue and pwait_queue are out of sync");
   }

#endif

   //-- NOTE: we should remove task from wait_queue before calling
   //   _on_task_wait_complete(), because _find_max_blocked_priority()
   //   in kernel_mutex.c checks for all tasks in mutex's wait_queue to
   //   get max blocked priority

   //-- NOTE: we don't care here whether task is contained in any wait_queue,
   //   because even if it isn't, _kernel_list_remove_entry() on empty list
   //   does just nothing.
   _kernel_list_remove_entry(&task->task_queue);
   //-- and reset task's queue
   _kernel_list_reset(&(task->task_queue));

   //-- handle current wait_reason: say, for MUTEX_I, we should
   //   handle priorities of other involved tasks.
   _on_task_wait_complete(task);

   task->pwait_queue  = KERNEL_NULL;
   task->task_wait_rc = wait_rc;

   //-- if timer is active (i.e. task waits for timeout),
   //   cancel that timer
   _kernel_timer_cancel(&task->timer);

   //-- remove WAIT state
   task->task_state &= ~KERNEL_TASK_STATE_WAIT;

   //-- Clear wait reason
   task->task_wait_reason = KERNEL_WAIT_REASON_NONE;
}

void _kernel_task_set_suspended(struct KERNEL_Task *task)
{
#if KERNEL_DEBUG
   //-- only WAIT bit is allowed here
   if (task->task_state & ~(KERNEL_TASK_STATE_WAIT)){
      _KERNEL_FATAL_ERROR("");
   }
#endif

   task->task_state |= KERNEL_TASK_STATE_SUSPEND;
}

void _kernel_task_clear_suspended(struct KERNEL_Task *task)
{
#if KERNEL_DEBUG
   //-- only WAIT and SUSPEND bits are allowed here,
   //   and SUSPEND bit must be set
   if (
         (task->task_state & ~(KERNEL_TASK_STATE_WAIT | KERNEL_TASK_STATE_SUSPEND))
         || (!(task->task_state &                   (KERNEL_TASK_STATE_SUSPEND)))
      )
   {
      _KERNEL_FATAL_ERROR("");
   }
#endif

   task->task_state &= ~KERNEL_TASK_STATE_SUSPEND;
}

void _kernel_task_set_dormant(struct KERNEL_Task* task)
{

#if KERNEL_DEBUG
   if (task->task_state != KERNEL_TASK_STATE_NONE){
      _KERNEL_FATAL_ERROR("");
   }
#if KERNEL_USE_MUTEXES
   else if (!_kernel_list_is_empty(&task->mutex_queue)){
      _KERNEL_FATAL_ERROR("");
   }
#if KERNEL_MUTEX_DEADLOCK_DETECT
   else if (!_kernel_list_is_empty(&task->deadlock_list)){
      _KERNEL_FATAL_ERROR("");
   }
#endif // KERNEL_MUTEX_DEADLOCK_DETECT
#endif // KERNEL_USE_MUTEXES
#endif // KERNEL_DEBUG

   task->priority    = task->base_priority;      //-- Task curr priority
   task->task_state  |= KERNEL_TASK_STATE_DORMANT;   //-- Task state

   task->tslice_count  = 0;
}

void _kernel_task_clear_dormant(struct KERNEL_Task *task)
{
#if KERNEL_DEBUG
   //-- task_state should be exactly KERNEL_TASK_STATE_DORMANT here
   if (task->task_state != KERNEL_TASK_STATE_DORMANT){
      _KERNEL_FATAL_ERROR("");
   }
#endif

   //--- Init task stack, save pointer to task top of stack,
   //    when not running
   task->stack_cur_pt = _kernel_arch_stack_init(
         task->task_func_addr,
         task->stack_low_addr,
         task->stack_high_addr,
         task->task_func_param
         );

   task->task_state &= ~KERNEL_TASK_STATE_DORMANT;


#if KERNEL_PROFILER
   //-- If profiler is present, set last tick count
   //   to current tick count value
   task->profiler.last_tick_cnt = _kernel_timer_sys_time_get();
#endif
}

/**
 * See comment in the _kernel_tasks.h file
 */
enum KERNEL_RCode _kernel_task_activate(struct KERNEL_Task *task)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (_kernel_task_is_dormant(task)){
      _kernel_task_clear_dormant(task);
      _kernel_task_set_runnable(task);
   } else {
      rc = KERNEL_RC_WSTATE;
   }

   return rc;
}

/**
 * See comment in the _kernel_tasks.h file
 */
KERNEL_BOOL _kernel_task_first_wait_complete(
      struct KERNEL_ListItem           *wait_queue,
      enum KERNEL_RCode                 wait_rc,
      _KERNEL_CBBeforeTaskWaitComplete *callback,
      void                         *user_data_1,
      void                         *user_data_2
      )
{
   KERNEL_BOOL ret = KERNEL_FALSE;

   if (!(_kernel_list_is_empty(wait_queue))){
      struct KERNEL_Task *task;
      //-- there are tasks in the wait queue, so, wake up the first one

      //-- get first task from the wait_queue
      task = _kernel_list_first_entry(wait_queue, struct KERNEL_Task, task_queue);

      //-- call provided callback (if any)
      if (callback != KERNEL_NULL){
         callback(task, user_data_1, user_data_2);
      }

      //-- wake task up
      _kernel_task_wait_complete(task, wait_rc);

      //-- indicate that some task has been woken up
      ret = KERNEL_TRUE;
   }

   return ret;
}


/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_change_task_priority(struct KERNEL_Task *task, int new_priority)
{
   if (_kernel_task_is_runnable(task)){
      _kernel_change_running_task_priority(task, new_priority);
   } else {
      task->priority = new_priority;
   }
}

/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_change_running_task_priority(struct KERNEL_Task *task, int new_priority)
{
   if (!_kernel_task_is_runnable(task)){
      _KERNEL_FATAL_ERROR("_kernel_change_running_task_priority called for non-runnable task");
   }

   //-- remove curr task from any (wait/ready) queue
   _remove_entry_from_ready_queue(&(task->task_queue), task->priority);

   task->priority = new_priority;

   //-- Add task to the end of ready queue for current priority
   _add_entry_to_ready_queue(&(task->task_queue), new_priority);

   _find_next_task_to_run();
}

#if 0
/**
 * See comment in the _kernel_tasks.h file
 */
void _kernel_task_set_last_rc_if_error(enum KERNEL_RCode rc)
{
   if (rc != KERNEL_RC_OK){
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();
      _kernel_curr_run_task->last_rc = rc;
      KERNEL_INT_RESTORE();
   }
}
#endif

#if KERNEL_USE_MUTEXES
/**
 * See comment in the _kernel_tasks.h file
 */
KERNEL_BOOL _kernel_is_mutex_locked_by_task(struct KERNEL_Task *task, struct KERNEL_Mutex *mutex)
{
   KERNEL_BOOL ret = KERNEL_FALSE;

   struct KERNEL_Mutex *tmp_mutex;
   _kernel_list_for_each_entry(
         tmp_mutex, struct KERNEL_Mutex, &(task->mutex_queue), mutex_queue
         )
   {
      if (tmp_mutex == mutex){
         ret = KERNEL_TRUE;
         break;
      }
   }

   return ret;
}

#endif

/*
 * See comment in the _kernel_tasks.h file
 */
void _kernel_task_exit_nodelete(void)
{
   kernel_task_exit((enum KERNEL_TaskExitOpt)(0));
}



#if !defined(_KERNEL_ARCH_STACK_DIR)
#  error _KERNEL_ARCH_STACK_DIR is not defined
#endif

#if (_KERNEL_ARCH_STACK_DIR == _KERNEL_ARCH_STACK_DIR__ASC)
//-- full ascending stack {{{

/*
 * See comments in the file _kernel_sys.h
 */
KERNEL_UWord *_kernel_task_stack_end_get(
      struct KERNEL_Task *task
      )
{
   return task->stack_high_addr;
}

// }}}
#elif (_KERNEL_ARCH_STACK_DIR == _KERNEL_ARCH_STACK_DIR__DESC)
//-- full descending stack {{{

/*
 * See comments in the file _kernel_tasks.h
 */
KERNEL_UWord *_kernel_task_stack_end_get(
      struct KERNEL_Task *task
      )
{
   return task->stack_low_addr;
}

// }}}
#else
#  error wrong _KERNEL_ARCH_STACK_DIR
#endif

