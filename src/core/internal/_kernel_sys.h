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

#ifndef __KERNEL_SYS_H
#define __KERNEL_SYS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_arch.h"
#include "kernel_list.h"
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

/// list of all ready to run (KERNEL_TASK_STATE_RUNNABLE) tasks
extern struct KERNEL_ListItem _kernel_tasks_ready_list[KERNEL_PRIORITIES_CNT];

/// list all created tasks (now it is used for statictic only)
extern struct KERNEL_ListItem _kernel_tasks_created_list;

/// count of created tasks
extern volatile int _kernel_tasks_created_cnt;

/// system state flags
extern volatile enum KERNEL_StateFlag _kernel_sys_state;

/// task that is running now
extern struct KERNEL_Task *_kernel_curr_run_task;

/// task that should run as soon as possible (if it isn't equal to
/// _kernel_curr_run_task, context switch is needed)
extern struct KERNEL_Task *_kernel_next_task_to_run;

/// bitmask of priorities with runnable tasks.
/// lowest priority bit (1 << (KERNEL_PRIORITIES_CNT - 1)) should always be set,
/// since this priority is used by idle task which should be always runnable,
/// by design.
extern volatile unsigned int _kernel_ready_to_run_bmp;

/// idle task structure
extern struct KERNEL_Task _kernel_idle_task;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#if !defined(container_of)
/* given a pointer @ptr to the field @member embedded into type (usually
 * struct) @type, return pointer to the embedding instance of @type. */
#define container_of(ptr, type, member) \
   ((type *)((char *)(ptr)-(char *)(&((type *)0)->member)))
#endif


//-- Check whether `KERNEL_DEBUG` is defined
//   (it must be defined to either 0 or 1)
#ifndef KERNEL_DEBUG
#  error KERNEL_DEBUG is not defined
#endif

//-- Depending on `KERNEL_DEBUG` value, define `_KERNEL_BUG_ON()` macro,
//   which generates runtime fatal error if given condition is true
#if KERNEL_DEBUG
#define  _KERNEL_BUG_ON(cond, ...){              \
   if (cond){                                \
      _KERNEL_FATAL_ERROR(__VA_ARGS__);          \
   }                                         \
}
#else
#define  _KERNEL_BUG_ON(cond)     /* `KERNEL_DEBUG` is 0, so, nothing to do here */
#endif




/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Remove all tasks from wait queue, returning the KERNEL_RC_DELETED code.
 */
void _kernel_wait_queue_notify_deleted(struct KERNEL_ListItem *wait_queue);


/**
 * Set system flags by bitmask.
 * Given flags value will be OR-ed with existing flags.
 *
 * @return previous _kernel_sys_state value.
 */
enum KERNEL_StateFlag _kernel_sys_state_flags_set(enum KERNEL_StateFlag flags);

/**
 * Clear flags by bitmask
 * Given flags value will be inverted and AND-ed with existing flags.
 *
 * @return previous _kernel_sys_state value.
 */
enum KERNEL_StateFlag _kernel_sys_state_flags_clear(enum KERNEL_StateFlag flags);

#if KERNEL_MUTEX_DEADLOCK_DETECT
/**
 * This function is called when deadlock becomes active or inactive
 * (this is detected by mutex subsystem).
 *
 * @param active
 *    Boolean value indicating whether deadlock becomes active or inactive.
 *    Note: deadlock might become inactive if, for example, one of tasks
 *    involved in deadlock exits from waiting by timeout.
 *
 * @param mutex
 *    mutex that is involved in deadlock. You may find out other mutexes
 *    involved by means of `mutex->deadlock_list`.
 *
 * @param task
 *    task that is involved in deadlock. You may find out other tasks involved
 *    by means of `task->deadlock_list`.
 *
 */
void _kernel_cry_deadlock(KERNEL_BOOL active, struct KERNEL_Mutex *mutex, struct KERNEL_Task *task);
#endif


#if _KERNEL_ON_CONTEXT_SWITCH_HANDLER
/**
 * This function is called at every context switch, if needed
 * (that is, if we have at least one on-context-switch handler:
 * say, profiler. See `#KERNEL_PROFILER`).
 *
 * It is a wrapper function which calls actual handlers, so that if we need
 * to add a new handler, we modify C code in just one place, instead of
 * modifying assembler code for each platform.
 *
 * @param task_prev
 *    Task that was running, and now it is going to wait
 * @param task_new
 *    Task that was waiting, and now it is going to run
 */
void _kernel_sys_on_context_switch(
      struct KERNEL_Task *task_prev, //-- task was running, going to wait
      struct KERNEL_Task *task_new   //-- task was waiting, going to run
      );
#endif



/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether context switch is needed (that is, if currently running task
 * is not the highest-priority task in the $(KERNEL_TASK_STATE_RUNNABLE) state)
 *
 * @return `#KERNEL_TRUE` if context switch is needed
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_need_context_switch(void)
{
   return (_kernel_curr_run_task != _kernel_next_task_to_run);
}

/**
 * If context switch is needed (see `#_kernel_need_context_switch()`),
 * context switch is pended (see `#_kernel_arch_context_switch_pend()`)
 */
_KERNEL_STATIC_INLINE void _kernel_context_switch_pend_if_needed(void)
{
   if (_kernel_need_context_switch()){
      _kernel_arch_context_switch_pend();
   }
}


#ifdef __cplusplus
}  /* extern "C" */
#endif


#ifdef __cplusplus
//-- C++: bitwise operators for enum types {{{
/**
 * Since the kernel should be able to compile by C++ compiler as well,
 * we have to explicitly write bitwise operators for enum types.
 *
 * NOTE that this file is an internal header, so these templates won't
 * pollute application namespace.
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator|=(T &a, int b)
{
   a = (T)((int)a | (int)b);
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator&=(T &a, int b)
{
   a = (T)((int)a & (int)b);
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator|(T a, int b)
{
   a |= b;
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator&(T a, int b)
{
   a &= b;
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator|=(T &a, T b)
{
   a = (T)((int)a | (int)b);
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator&=(T &a, T b)
{
   a = (T)((int)a & (int)b);
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator|(T a, T b)
{
   a |= b;
   return a;
}

/**
 * See comments for `operator|=` above
 */
template <typename T>
_KERNEL_STATIC_INLINE T operator&(T a, T b)
{
   a &= b;
   return a;
}
// }}}
#endif

#endif // __KERNEL_SYS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


