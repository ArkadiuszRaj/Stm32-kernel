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
 * Kernel system routines.
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_common.h"
#include "kernel_sys.h"

//-- internal kernelkernel headers
#include "_kernel_sys.h"
#include "_kernel_timer.h"
#include "_kernel_tasks.h"
#include "_kernel_list.h"


#include "kernel_tasks.h"
#include "kernel_timer.h"


//-- for memcmp() and memset() that is used inside the macro
//   `_KERNEL_BUILD_CFG_STRUCT_FILL()`
#include <string.h>

//-- self-check
#if !defined(KERNEL_PRIORITIES_MAX_CNT)
#  error KERNEL_PRIORITIES_MAX_CNT is not defined
#endif

//-- check KERNEL_PRIORITIES_CNT
#if (KERNEL_PRIORITIES_CNT > KERNEL_PRIORITIES_MAX_CNT)
#  error KERNEL_PRIORITIES_CNT is too large (maximum is KERNEL_PRIORITIES_MAX_CNT)
#endif


/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/

// See comments in the internal/_kernel_sys.h file
struct KERNEL_ListItem _kernel_tasks_ready_list[KERNEL_PRIORITIES_CNT];

// See comments in the internal/_kernel_sys.h file
struct KERNEL_ListItem _kernel_tasks_created_list;

// See comments in the internal/_kernel_sys.h file
volatile int _kernel_tasks_created_cnt;

// See comments in the internal/_kernel_sys.h file
volatile enum KERNEL_StateFlag _kernel_sys_state;

// See comments in the internal/_kernel_sys.h file
struct KERNEL_Task *_kernel_next_task_to_run;

// See comments in the internal/_kernel_sys.h file
struct KERNEL_Task *_kernel_curr_run_task;

// See comments in the internal/_kernel_sys.h file
volatile unsigned int _kernel_ready_to_run_bmp;

// See comments in the internal/_kernel_sys.h file
struct KERNEL_Task _kernel_idle_task;





/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

/*
 * NOTE: as long as these variables are private, they could be declared as
 * `static` actually, but for easier debug they are left global.
 */


/// Pointer to user idle callback function, it gets called regularly
/// from the idle task.
KERNEL_CBIdle *_kernel_cb_idle_hook = KERNEL_NULL;

/// Pointer to stack overflow callback function. When stack overflow
/// is detected by the kernel, this function gets called.
/// (see `#KERNEL_STACK_OVERFLOW_CHECK`)
KERNEL_CBStackOverflow *_kernel_cb_stack_overflow = KERNEL_NULL;

/// User-provided callback function that gets called whenever
/// mutex deadlock occurs.
/// (see `#KERNEL_MUTEX_DEADLOCK_DETECT`)
KERNEL_CBDeadlock *_kernel_cb_deadlock = KERNEL_NULL;

/// Time slice values for each available priority, in system ticks.
unsigned short _kernel_tslice_ticks[KERNEL_PRIORITIES_CNT];

#if KERNEL_MUTEX_DEADLOCK_DETECT
/// Number of deadlocks active at the moment. Normally it is equal to 0.
int _kernel_deadlocks_cnt = 0;
#endif


/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/



/*******************************************************************************
 *    EXTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/

extern const struct _KERNEL_BuildCfg *kernel_app_build_cfg_get(void);
extern void you_should_add_file___kernel_app_check_c___to_the_project(void);



/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Idle task body. In fact, this task is always in RUNNABLE state.
 */
static void _idle_task_body(void *par)
{
   //-- enter endless loop with calling user-provided hook function
   for(;;)
   {
      _kernel_cb_idle_hook();
   }
   _KERNEL_UNUSED(par);
}

/**
 * Manage round-robin (if used)
 */
#if KERNEL_DYNAMIC_TICK

_KERNEL_STATIC_INLINE void _round_robin_manage(void) {
   /*TODO: round-robin should be powered by timers mechanism.
    * So, when round-robin is active, the system isn't so "tickless".
    */
}

#else

_KERNEL_STATIC_INLINE void _round_robin_manage(void)
{
   //-- Manage round robin if only context switch is not already needed for
   //   some other reason
   if (_kernel_curr_run_task == _kernel_next_task_to_run) {
      //-- volatile is used here only to solve
      //   IAR(c) compiler's high optimization mode problem
      _KERNEL_VOLATILE_WORKAROUND struct KERNEL_ListItem *curr_que;
      _KERNEL_VOLATILE_WORKAROUND struct KERNEL_ListItem *pri_queue;
      _KERNEL_VOLATILE_WORKAROUND int priority = _kernel_curr_run_task->priority;

      if (_kernel_tslice_ticks[priority] != KERNEL_NO_TIME_SLICE){
         _kernel_curr_run_task->tslice_count++;

         if (_kernel_curr_run_task->tslice_count >= _kernel_tslice_ticks[priority]){
            _kernel_curr_run_task->tslice_count = 0;

            pri_queue = &(_kernel_tasks_ready_list[priority]);
            //-- If ready queue is not empty and there are more than 1
            //   task in the queue
            if (     !(_kernel_list_is_empty((struct KERNEL_ListItem *)pri_queue))
                  && pri_queue->next->next != pri_queue
               )
            {
               //-- Remove task from head and add it to the tail of
               //-- ready queue for current priority

               curr_que = _kernel_list_remove_head(&(_kernel_tasks_ready_list[priority]));
               _kernel_list_add_tail(&(_kernel_tasks_ready_list[priority]), curr_que);

               _kernel_next_task_to_run = _kernel_get_task_by_tsk_queue(
                     _kernel_tasks_ready_list[priority].next
                     );
            }
         }
      }
   }
}

#endif


#if _KERNEL_ON_CONTEXT_SWITCH_HANDLER
#if KERNEL_PROFILER
/**
 * This function is called at every context switch, if `#KERNEL_PROFILER` is
 * non-zero.
 *
 * @param task_prev
 *    Task that was running, and now it is going to wait
 * @param task_new
 *    Task that was waiting, and now it is going to run
 */
_KERNEL_STATIC_INLINE void _kernel_sys_on_context_switch_profiler(
      struct KERNEL_Task *task_prev,
      struct KERNEL_Task *task_new
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON(!KERNEL_IS_INT_DISABLED());

   KERNEL_TickCnt cur_tick_cnt = _kernel_timer_sys_time_get();

   //-- handle task_prev (the one that was running and going to wait) {{{
   {
#if KERNEL_DEBUG
      if (!task_prev->profiler.is_running){
         _KERNEL_FATAL_ERROR();
      }
      task_prev->profiler.is_running = 0;
#endif

      //-- get difference between current time and last saved time:
      //   this is the time task was running.
      KERNEL_TickCnt cur_run_time
         = (KERNEL_TickCnt)(cur_tick_cnt - task_prev->profiler.last_tick_cnt);

      //-- add it to total run time
      task_prev->profiler.timing.total_run_time += cur_run_time;

      //-- check if we should update consecutive max run time
      if (task_prev->profiler.timing.max_consecutive_run_time < cur_run_time){
         task_prev->profiler.timing.max_consecutive_run_time = cur_run_time;
      }

      //-- update current task state
      task_prev->profiler.last_tick_cnt      = cur_tick_cnt;
#if KERNEL_PROFILER_WAIT_TIME
      task_prev->profiler.last_wait_reason   = task_prev->task_wait_reason;
#endif
   }
   // }}}

   //-- handle task_new (the one that was waiting and going to run) {{{
   {
#if KERNEL_DEBUG
      if (task_new->profiler.is_running){
         _KERNEL_FATAL_ERROR();
      }
      task_new->profiler.is_running = 1;
#endif

#if KERNEL_PROFILER_WAIT_TIME
      //-- get difference between current time and last saved time:
      //   this is the time task was waiting.
      KERNEL_TickCnt cur_wait_time
         = (KERNEL_TickCnt)(cur_tick_cnt - task_new->profiler.last_tick_cnt);

      //-- add it to total total_wait_time for particular wait reason
      task_new->profiler.timing.total_wait_time
         [ task_new->profiler.last_wait_reason ]
         += cur_wait_time;

      //-- check if we should update consecutive max wait time
      if (
            task_new->profiler.timing.max_consecutive_wait_time
            [ task_new->profiler.last_wait_reason ] < cur_wait_time
         )
      {
         task_new->profiler.timing.max_consecutive_wait_time
            [ task_new->profiler.last_wait_reason ] = cur_wait_time;
      }
#endif

      //-- increment the counter of times task got running
      task_new->profiler.timing.got_running_cnt++;

      //-- update current task state
      task_new->profiler.last_tick_cnt      = cur_tick_cnt;
   }
   // }}}
}
#else

/**
 * Stub empty function, it is needed when `#KERNEL_PROFILER` is zero.
 */
_KERNEL_STATIC_INLINE void _kernel_sys_on_context_switch_profiler(
      struct KERNEL_Task *task_prev, //-- task was running, going to wait
      struct KERNEL_Task *task_new   //-- task was waiting, going to run
      )
{
   _KERNEL_UNUSED(task_prev);
   _KERNEL_UNUSED(task_new);
}
#endif
#endif


#if KERNEL_STACK_OVERFLOW_CHECK
/**
 * if `#KERNEL_STACK_OVERFLOW_CHECK` is non-zero, this function is called at every
 * context switch as well as inside `#kernel_tick_int_processing()`.
 *
 * It merely checks whether stack bottom is still untouched (that is, whether
 * it has the value `KERNEL_FILL_STACK_VAL`).
 *
 * If the value is corrupted, the function calls user-provided callback
 * `_kernel_cb_stack_overflow()`. If user hasn't provided that callback,
 * `#_KERNEL_FATAL_ERROR()` is called.
 *
 * @param task
 *    Task to check
 */
_KERNEL_STATIC_INLINE void _kernel_sys_stack_overflow_check(
      struct KERNEL_Task *task
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON(!KERNEL_IS_INT_DISABLED());

   //-- check that stack bottom has the value `KERNEL_FILL_STACK_VAL`

   KERNEL_UWord *p_word = _kernel_task_stack_end_get(task);

   if (*p_word != KERNEL_FILL_STACK_VAL){
      //-- stack overflow is detected, so, notify the user about that.

      if (_kernel_cb_stack_overflow != NULL){
         _kernel_cb_stack_overflow(task);
      } else {
         _KERNEL_FATAL_ERROR("stack overflow");
      }
   }
}
#else

/**
 * Stub empty function, it is needed when `#KERNEL_STACK_OVERFLOW_CHECK` is zero.
 */
_KERNEL_STATIC_INLINE void _kernel_sys_stack_overflow_check(
      struct KERNEL_Task *task
      )
{
   _KERNEL_UNUSED(task);
}
#endif

/**
 * Create idle task, the task is NOT started after creation.
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _idle_task_create(
      KERNEL_UWord      *idle_task_stack,
      unsigned int   idle_task_stack_size
      )
{
   return kernel_task_create_wname(
         (struct KERNEL_Task*)&_kernel_idle_task, //-- task TCB
         _idle_task_body,                 //-- task function
         KERNEL_PRIORITIES_CNT - 1,           //-- task priority
         idle_task_stack,                 //-- task stack
         idle_task_stack_size,            //-- task stack size
                                          //   (in int, not bytes)
         KERNEL_NULL,                         //-- task function parameter
         (_KERNEL_TASK_CREATE_OPT_IDLE),      //-- Creation option
         "Idle"                           //-- Task name
         );
}

/**
 * If enabled, define a function which ensures that build-time options for the
 * kernel match the ones for the application.
 *
 * See `#KERNEL_CHECK_BUILD_CFG` for details.
 */
#if KERNEL_CHECK_BUILD_CFG
static void _build_cfg_check(void)
{
   struct _KERNEL_BuildCfg kernel_build_cfg;
   _KERNEL_BUILD_CFG_STRUCT_FILL(&kernel_build_cfg);

   //-- call dummy function that helps user to understand that
   //   he/she forgot to add file kernel_app_check.c to the project
   you_should_add_file___kernel_app_check_c___to_the_project();

   //-- get application build cfg
   const struct _KERNEL_BuildCfg *app_build_cfg = kernel_app_build_cfg_get();

   //-- now, check each option

   if (kernel_build_cfg.priorities_cnt != app_build_cfg->priorities_cnt){
      _KERNEL_FATAL_ERROR("KERNEL_PRIORITIES_CNT doesn't match");
   }

   if (kernel_build_cfg.check_param != app_build_cfg->check_param){
      _KERNEL_FATAL_ERROR("KERNEL_CHECK_PARAM doesn't match");
   }

   if (kernel_build_cfg.debug != app_build_cfg->debug){
      _KERNEL_FATAL_ERROR("KERNEL_DEBUG doesn't match");
   }

   if (kernel_build_cfg.use_mutexes != app_build_cfg->use_mutexes){
      _KERNEL_FATAL_ERROR("KERNEL_USE_MUTEXES doesn't match");
   }

   if (kernel_build_cfg.mutex_rec != app_build_cfg->mutex_rec){
      _KERNEL_FATAL_ERROR("KERNEL_MUTEX_REC doesn't match");
   }

   if (kernel_build_cfg.mutex_deadlock_detect != app_build_cfg->mutex_deadlock_detect){
      _KERNEL_FATAL_ERROR("KERNEL_MUTEX_DEADLOCK_DETECT doesn't match");
   }

   if (kernel_build_cfg.tick_lists_cnt_minus_one != app_build_cfg->tick_lists_cnt_minus_one){
      _KERNEL_FATAL_ERROR("KERNEL_TICK_LISTS_CNT doesn't match");
   }

   if (kernel_build_cfg.api_make_alig_arg != app_build_cfg->api_make_alig_arg){
      _KERNEL_FATAL_ERROR("KERNEL_API_MAKE_ALIG_ARG doesn't match");
   }

   if (kernel_build_cfg.profiler != app_build_cfg->profiler){
      _KERNEL_FATAL_ERROR("KERNEL_PROFILER doesn't match");
   }

   if (kernel_build_cfg.profiler_wait_time != app_build_cfg->profiler_wait_time){
      _KERNEL_FATAL_ERROR("KERNEL_PROFILER_WAIT_TIME doesn't match");
   }

   if (kernel_build_cfg.stack_overflow_check != app_build_cfg->stack_overflow_check){
      _KERNEL_FATAL_ERROR("KERNEL_STACK_OVERFLOW_CHECK doesn't match");
   }

   if (kernel_build_cfg.dynamic_tick != app_build_cfg->dynamic_tick){
      _KERNEL_FATAL_ERROR("KERNEL_DYNAMIC_TICK doesn't match");
   }

   if (kernel_build_cfg.old_events_api != app_build_cfg->old_events_api){
      _KERNEL_FATAL_ERROR("KERNEL_OLD_EVENT_API doesn't match");
   }

#if defined (__KERNEL_ARCH_PIC24_DSPIC__)
   if (kernel_build_cfg.arch.p24.p24_sys_ipl != app_build_cfg->arch.p24.p24_sys_ipl){
      _KERNEL_FATAL_ERROR("KERNEL_P24_SYS_IPL doesn't match");
   }
#endif


   //-- for the case I forgot to add some param above, perform generic check
   KERNEL_BOOL cfg_match =
      !memcmp(&kernel_build_cfg, app_build_cfg, sizeof(kernel_build_cfg));

   if (!cfg_match){
      _KERNEL_FATAL_ERROR("configuration mismatch");
   }
}
#else

static inline void _build_cfg_check(void) {}

#endif




/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (kernel_sys.h)
 */
void kernel_sys_start(
      KERNEL_UWord            *idle_task_stack,
      unsigned int         idle_task_stack_size,
      KERNEL_UWord            *int_stack,
      unsigned int         int_stack_size,
      KERNEL_CBUserTaskCreate *cb_user_task_create,
      KERNEL_CBIdle           *cb_idle
      )
{
   unsigned int i;
   enum KERNEL_RCode rc;

   //-- init timers
   _kernel_timers_init();

   //-- check that build configuration for the kernel and application match
   //   (if only KERNEL_CHECK_BUILD_CFG is non-zero)
   _build_cfg_check();

   //-- for each priority:
   //   - reset list of runnable tasks with this priority
   //   - reset time slice to `#KERNEL_NO_TIME_SLICE`
   for (i = 0; i < KERNEL_PRIORITIES_CNT; i++){
      _kernel_list_reset(&(_kernel_tasks_ready_list[i]));
      _kernel_tslice_ticks[i] = KERNEL_NO_TIME_SLICE;
   }

   //-- reset generic task queue and task count to 0
   _kernel_list_reset(&_kernel_tasks_created_list);
   _kernel_tasks_created_cnt = 0;

   //-- initial system flags: no flags set (see enum KERNEL_StateFlag)
   _kernel_sys_state = (enum KERNEL_StateFlag)(0);

   //-- reset bitmask of priorities with runnable tasks
   _kernel_ready_to_run_bmp = 0;

   //-- reset pointers to currently running task and next task to run
   _kernel_next_task_to_run = KERNEL_NULL;
   _kernel_curr_run_task    = KERNEL_NULL;

   //-- remember user-provided callbacks
   _kernel_cb_idle_hook = cb_idle;

   //-- Fill interrupt stack space with KERNEL_FILL_STACK_VAL
#if KERNEL_INIT_INTERRUPT_STACK_SPACE
   for (i = 0; i < int_stack_size; i++){
      int_stack[i] = KERNEL_FILL_STACK_VAL;
   }
#endif

   /*
    * NOTE: we need to separate creation of tasks and making them runnable,
    *       because otherwise _kernel_next_task_to_run would point on the task
    *       that isn't yet created, and it produces issues
    *       with order of task creation.
    *
    *       We should keep as little surprizes in the code as possible,
    *       so, it's better to just separate these steps and avoid any tricks.
    */

   //-- create system tasks
   rc = _idle_task_create(idle_task_stack, idle_task_stack_size);
   if (rc != KERNEL_RC_OK){
      _KERNEL_FATAL_ERROR("failed to create idle task");
   }

   //-- Just for the _kernel_task_set_runnable() proper operation
   _kernel_next_task_to_run = &_kernel_idle_task;

   //-- make system tasks runnable
   rc = _kernel_task_activate(&_kernel_idle_task);
   if (rc != KERNEL_RC_OK){
      _KERNEL_FATAL_ERROR("failed to activate idle task");
   }

   //-- set _kernel_curr_run_task to idle task
   _kernel_curr_run_task = &_kernel_idle_task;
#if KERNEL_PROFILER
#if KERNEL_DEBUG
   _kernel_idle_task.profiler.is_running = 1;
#endif
#endif

   //-- now, we can create user's task(s)
   //   (by user-provided callback)
   cb_user_task_create();

   //-- set flag that system is running
   //   (well, it will be running soon actually)
   _kernel_sys_state |= KERNEL_STATE_FLAG__SYS_RUNNING;

   //-- call architecture-dependent initialization and run the kernel:
   //   (perform first context switch)
   _kernel_arch_sys_start(int_stack, int_stack_size);

   //-- should never be here
   _KERNEL_FATAL_ERROR("should never be here");
}



/*
 * See comments in the header file (kernel_sys.h)
 */
void kernel_tick_int_processing(void)
{
   KERNEL_INTSAVE_DATA_INT;

   KERNEL_INT_IDIS_SAVE();

   //-- check stack overflow
   _kernel_sys_stack_overflow_check(_kernel_curr_run_task);

   //-- manage timers
   _kernel_timers_tick_proceed(KERNEL_INTSAVE_VAR);

   //-- manage round-robin (if used)
   _round_robin_manage();

   KERNEL_INT_IRESTORE();
   _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();
}

/*
 * See comments in the header file (kernel_sys.h)
 */
enum KERNEL_RCode kernel_sys_tslice_set(int priority, int ticks)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else if (0
         || priority < 0 || priority >= (KERNEL_PRIORITIES_CNT - 1)
         || ticks    < 0 || ticks    >   KERNEL_MAX_TIME_SLICE)
   {
      rc = KERNEL_RC_WPARAM;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();
      _kernel_tslice_ticks[priority] = ticks;
      KERNEL_INT_RESTORE();
   }
   return rc;
}

/*
 * See comments in the header file (kernel_sys.h)
 */
KERNEL_TickCnt kernel_sys_time_get(void)
{
   KERNEL_TickCnt ret;
   KERNEL_INTSAVE_DATA;

   KERNEL_INT_DIS_SAVE();
   ret = _kernel_timer_sys_time_get();
   KERNEL_INT_RESTORE();

   return ret;
}

/*
 * Returns current state flags (_kernel_sys_state)
 */
enum KERNEL_StateFlag kernel_sys_state_flags_get(void)
{
   return _kernel_sys_state;
}

/*
 * See comment in kernel_sys.h file
 */
void kernel_callback_deadlock_set(KERNEL_CBDeadlock *cb)
{
   _kernel_cb_deadlock = cb;
}

/*
 * See comment in kernel_sys.h file
 */
void kernel_callback_stack_overflow_set(KERNEL_CBStackOverflow *cb)
{
   _kernel_cb_stack_overflow = cb;
}

/*
 * See comment in kernel_sys.h file
 */
enum KERNEL_Context kernel_sys_context_get(void)
{
   enum KERNEL_Context ret;

   if (_kernel_sys_state & KERNEL_STATE_FLAG__SYS_RUNNING){
      ret = _kernel_arch_inside_isr()
         ? KERNEL_CONTEXT_ISR
         : KERNEL_CONTEXT_TASK;
   } else {
      ret = KERNEL_CONTEXT_NONE;
   }

   return ret;
}

/*
 * See comment in kernel_sys.h file
 */
struct KERNEL_Task *kernel_cur_task_get(void)
{
   return _kernel_curr_run_task;
}

/*
 * See comment in kernel_sys.h file
 */
KERNEL_TaskBody *kernel_cur_task_body_get(void)
{
   return _kernel_curr_run_task->task_func_addr;
}


#if KERNEL_DYNAMIC_TICK

void kernel_callback_dyn_tick_set(
      KERNEL_CBTickSchedule   *cb_tick_schedule,
      KERNEL_CBTickCntGet     *cb_tick_cnt_get
      )
{
   _kernel_timer_dyn_callback_set(cb_tick_schedule, cb_tick_cnt_get);
}

#endif




/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comment in the _kernel_sys.h file
 */
void _kernel_wait_queue_notify_deleted(struct KERNEL_ListItem *wait_queue)
{
   struct KERNEL_Task *task;         //-- "cursor" for the loop iteration
   struct KERNEL_Task *tmp_task;     //-- we need for temporary item because
                                 //   item is removed from the list
                                 //   in _kernel_mutex_do_unlock().


   //-- iterate through all tasks in the wait_queue,
   //   calling _kernel_task_wait_complete() for each task,
   //   and setting KERNEL_RC_DELETED as a wait return code.
   _kernel_list_for_each_entry_safe(
         task, struct KERNEL_Task, tmp_task, wait_queue, task_queue
         )
   {
      //-- call _kernel_task_wait_complete for every task
      _kernel_task_wait_complete(task, KERNEL_RC_DELETED);
   }

#if KERNEL_DEBUG
   if (!_kernel_list_is_empty(wait_queue)){
      _KERNEL_FATAL_ERROR("");
   }
#endif
}

/**
 * See comments in the file _kernel_sys.h
 */
enum KERNEL_StateFlag _kernel_sys_state_flags_set(enum KERNEL_StateFlag flags)
{
   enum KERNEL_StateFlag ret = _kernel_sys_state;
   _kernel_sys_state |= flags;
   return ret;
}

/**
 * See comments in the file _kernel_sys.h
 */
enum KERNEL_StateFlag _kernel_sys_state_flags_clear(enum KERNEL_StateFlag flags)
{
   enum KERNEL_StateFlag ret = _kernel_sys_state;
   _kernel_sys_state &= ~flags;
   return ret;
}


#if KERNEL_MUTEX_DEADLOCK_DETECT
/**
 * See comments in the file _kernel_sys.h
 */
void _kernel_cry_deadlock(KERNEL_BOOL active, struct KERNEL_Mutex *mutex, struct KERNEL_Task *task)
{
   if (active){
      //-- deadlock just became active
      if (_kernel_deadlocks_cnt == 0){
         _kernel_sys_state_flags_set(KERNEL_STATE_FLAG__DEADLOCK);
      }

      _kernel_deadlocks_cnt++;
   } else {
      //-- deadlock just became inactive
      _kernel_deadlocks_cnt--;

      if (_kernel_deadlocks_cnt == 0){
         _kernel_sys_state_flags_clear(KERNEL_STATE_FLAG__DEADLOCK);
      }
   }

   //-- if user has specified callback function for deadlock detection,
   //   notify him by calling this function
   if (_kernel_cb_deadlock != KERNEL_NULL){
      _kernel_cb_deadlock(active, mutex, task);
   }

}
#endif

#if _KERNEL_ON_CONTEXT_SWITCH_HANDLER
/*
 * See comments in the file _kernel_sys.h
 */
void _kernel_sys_on_context_switch(
      struct KERNEL_Task *task_prev,
      struct KERNEL_Task *task_new
      )
{
   _kernel_sys_stack_overflow_check(task_prev);
   _kernel_sys_on_context_switch_profiler(task_prev, task_new);
}
#endif





