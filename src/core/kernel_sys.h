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
 * Kernel system routines: system start, tick processing, time slice managing.
 *
 */

#ifndef _KERNEL_SYS_H
#define _KERNEL_SYS_H



/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_list.h"
#include "../arch/kernel_arch.h"
#include "kernel_cfg_dispatch.h"

#include "kernel_timer.h"



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    EXTERNAL TYPES
 ******************************************************************************/

struct KERNEL_Task;
struct KERNEL_Mutex;



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Convenience macro for the definition of stack array. See
 * `kernel_task_create()` for the usage example.
 *
 * @param name
 *    C variable name of the array
 * @param size
 *    size of the stack array in words (`#KERNEL_UWord`), not in bytes.
 */
#define  KERNEL_STACK_ARR_DEF(name, size)        \
   KERNEL_ARCH_STK_ATTR_BEFORE                   \
   KERNEL_UWord name[ (size) ]                   \
   KERNEL_ARCH_STK_ATTR_AFTER


#if KERNEL_CHECK_BUILD_CFG

/**
 * For internal kernel usage: helper macro that fills architecture-dependent
 * values. This macro is used by `#_KERNEL_BUILD_CFG_STRUCT_FILL()` only.
 */
#if defined (__KERNEL_ARCH_PIC24_DSPIC__)

#  define _KERNEL_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct)               \
{                                                                 \
   (_p_struct)->arch.p24.p24_sys_ipl = KERNEL_P24_SYS_IPL;            \
}

#else
#  define _KERNEL_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct)
#endif




/**
 * For internal kernel usage: fill the structure `#_KERNEL_BuildCfg` with
 * current build-time configuration values.
 *
 * @param _p_struct     Pointer to struct `#_KERNEL_BuildCfg`
 */
#define  _KERNEL_BUILD_CFG_STRUCT_FILL(_p_struct)                           \
{                                                                       \
   memset((_p_struct), 0x00, sizeof(*(_p_struct)));                     \
                                                                        \
   (_p_struct)->priorities_cnt            = KERNEL_PRIORITIES_CNT;          \
   (_p_struct)->check_param               = KERNEL_CHECK_PARAM;             \
   (_p_struct)->debug                     = KERNEL_DEBUG;                   \
   (_p_struct)->use_mutexes               = KERNEL_USE_MUTEXES;             \
   (_p_struct)->mutex_rec                 = KERNEL_MUTEX_REC;               \
   (_p_struct)->mutex_deadlock_detect     = KERNEL_MUTEX_DEADLOCK_DETECT;   \
   (_p_struct)->tick_lists_cnt_minus_one  = (KERNEL_TICK_LISTS_CNT - 1);    \
   (_p_struct)->api_make_alig_arg         = KERNEL_API_MAKE_ALIG_ARG;       \
   (_p_struct)->profiler                  = KERNEL_PROFILER;                \
   (_p_struct)->profiler_wait_time        = KERNEL_PROFILER_WAIT_TIME;      \
   (_p_struct)->stack_overflow_check      = KERNEL_STACK_OVERFLOW_CHECK;    \
   (_p_struct)->dynamic_tick              = KERNEL_DYNAMIC_TICK;            \
   (_p_struct)->old_events_api            = KERNEL_OLD_EVENT_API;           \
                                                                        \
   _KERNEL_BUILD_CFG_ARCH_STRUCT_FILL(_p_struct);                           \
}


#else

#define  _KERNEL_BUILD_CFG_STRUCT_FILL(_p_struct)   /* nothing */

#endif   // KERNEL_CHECK_BUILD_CFG


/**
 * For internal kernel usage: helper macro that allows functions to be inlined
 * or not depending on configuration (see `#KERNEL_MAX_INLINE`)
 */
#if KERNEL_MAX_INLINE
#  define _KERNEL_MAX_INLINED_FUNC _KERNEL_STATIC_INLINE
#else
#  define _KERNEL_MAX_INLINED_FUNC /* nothing */
#endif



/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Structure with build-time configurations values; it is needed for run-time
 * check which ensures that build-time options for the kernel match ones for
 * the application. See `#KERNEL_CHECK_BUILD_CFG` for details.
 */
struct _KERNEL_BuildCfg {
   ///
   /// Value of `#KERNEL_PRIORITIES_CNT`
   unsigned          priorities_cnt             : 7;
   ///
   /// Value of `#KERNEL_CHECK_PARAM`
   unsigned          check_param                : 1;
   ///
   /// Value of `#KERNEL_DEBUG`
   unsigned          debug                      : 1;
   //-- Note: we don't include KERNEL_OLD_KERNELKERNEL_NAMES since it doesn't
   //   affect behavior of the kernel in any way.
   ///
   /// Value of `#KERNEL_USE_MUTEXES`
   unsigned          use_mutexes                : 1;
   ///
   /// Value of `#KERNEL_MUTEX_REC`
   unsigned          mutex_rec                  : 1;
   ///
   /// Value of `#KERNEL_MUTEX_DEADLOCK_DETECT`
   unsigned          mutex_deadlock_detect      : 1;
   ///
   /// Value of `#KERNEL_TICK_LISTS_CNT` minus one
   unsigned          tick_lists_cnt_minus_one   : 8;
   ///
   /// Value of `#KERNEL_API_MAKE_ALIG_ARG`
   unsigned          api_make_alig_arg          : 2;
   ///
   /// Value of `#KERNEL_PROFILER`
   unsigned          profiler                   : 1;
   ///
   /// Value of `#KERNEL_PROFILER_WAIT_TIME`
   unsigned          profiler_wait_time         : 1;
   ///
   /// Value of `#KERNEL_STACK_OVERFLOW_CHECK`
   unsigned          stack_overflow_check       : 1;
   ///
   /// Value of `#KERNEL_DYNAMIC_TICK`
   unsigned          dynamic_tick               : 1;
   ///
   /// Value of `#KERNEL_OLD_EVENT_API`
   unsigned          old_events_api             : 1;
   ///
   /// Architecture-dependent values
   union {
      ///
      /// On some architectures, we don't have any arch-dependent build-time
      /// options, but we need this "dummy" value to avoid errors of crappy
      /// compilers that don't allow empty structure initializers (like ARMCC)
      KERNEL_UWord dummy;
      ///
      /// PIC24/dsPIC-dependent values
      struct {
         ///
         /// Value of `#KERNEL_P24_SYS_IPL`
         unsigned    p24_sys_ipl                : 3;
      } p24;
   } arch;
};

/**
 * System state flags
 */
enum KERNEL_StateFlag {
   ///
   /// system is running
   KERNEL_STATE_FLAG__SYS_RUNNING    = (1 << 0),
   ///
   /// deadlock is active
   /// Note: this feature works if only `#KERNEL_MUTEX_DEADLOCK_DETECT` is non-zero.
   /// @see `#KERNEL_MUTEX_DEADLOCK_DETECT`
   KERNEL_STATE_FLAG__DEADLOCK       = (1 << 1),
};

/**
 * System context
 *
 * @see `kernel_sys_context_get()`
 */
enum KERNEL_Context {
   ///
   /// None: this code is possible if only system is not running
   /// (flag (`#KERNEL_STATE_FLAG__SYS_RUNNING` is not set in the `_kernel_sys_state`))
   KERNEL_CONTEXT_NONE,
   ///
   /// Task context
   KERNEL_CONTEXT_TASK,
   ///
   /// ISR context
   KERNEL_CONTEXT_ISR,
};

/**
 * User-provided callback function that is called directly from
 * `kernel_sys_start()` as a part of system startup routine; it should merely
 * create at least one (and typically just one) user's task, which should
 * perform all the rest application initialization.
 *
 * When `KERNEL_CBUserTaskCreate()` returned, the kernel performs first context
 * switch to the task with highest priority. If there are several tasks with
 * highest priority, context is switched to the first created one.
 *
 * Refer to the section \ref starting_the_kernel for details about system
 * startup process on the whole.
 *
 * **Note:** Although you're able to create more than one task here, it's
 * usually not so good idea, because many things typically should be done at
 * startup before tasks can go on with their job: we need to initialize various
 * on-board peripherals (displays, flash memory chips, or whatever) as well as
 * initialize software modules used by application. So, if many tasks are
 * created here, you have to provide some synchronization object so that tasks
 * will wait until all the initialization is done.
 *
 * It's usually easier to maintain if we create just one task here, which
 * firstly performs all the necessary initialization, **then** creates the rest
 * of your tasks, and eventually gets to its primary job (the job for which
 * task was created at all). For the usage example, refer to the page \ref
 * starting_the_kernel.
 *
 * \attention
 *    * The only system service is allowed to call in this function is
 *      `kernel_task_create()`.
 *
 * @see `kernel_sys_start()`
 */
typedef void (KERNEL_CBUserTaskCreate)(void);

/**
 * User-provided callback function which is called repeatedly from the idle
 * task loop. Make sure that idle task has enough stack space to call this
 * function.
 *
 * Typically, this callback can be used for things like:
 *
 *   * MCU sleep/idle mode. When system has nothing to do, it often makes sense
 *     to bring processor to some power-saving mode. Of course, the application
 *     is responsible for setting some condition to wake up: typically, it's an
 *     interrupt.
 *   * Calculation of system load. The easiest implementation is to just
 *     increment some variable in the idle task. The faster value grows, the
 *     less busy system is.
 *
 * \attention
 *    * From withing this callback, it is illegal to invoke `#kernel_task_sleep()`
 *      or any other service which could put task to waiting state, because
 *      idle task (from which this function is called) should always be
 *      runnable, by design. If `#KERNEL_DEBUG` option is set, then this is
 *      checked, so if idle task becomes non-runnable, `_KERNEL_FATAL_ERROR()`
 *      macro will be called.
 *
 * @see `kernel_sys_start()`
 */
typedef void (KERNEL_CBIdle)(void);

/**
 * User-provided callback function that is called when the kernel detects stack
 * overflow (see `#KERNEL_STACK_OVERFLOW_CHECK`).
 *
 * @param task
 *    Task whose stack is overflowed
 */
typedef void (KERNEL_CBStackOverflow)(struct KERNEL_Task *task);

/**
 * User-provided callback function that is called whenever
 * deadlock becomes active or inactive.
 * Note: this feature works if only `#KERNEL_MUTEX_DEADLOCK_DETECT` is non-zero.
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
 */
typedef void (KERNEL_CBDeadlock)(
      KERNEL_BOOL active,
      struct KERNEL_Mutex *mutex,
      struct KERNEL_Task *task
      );




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Value to pass to `kernel_sys_tslice_set()` to turn round-robin off.
 */
#define  KERNEL_NO_TIME_SLICE              0

/**
 * Max value of time slice
 */
#define  KERNEL_MAX_TIME_SLICE             0xFFFE




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Initial KERNEL system start function, never returns. Typically called
 * from main().
 *
 * Refer to the \ref starting_the_kernel "Starting the kernel" section for the
 * usage example and additional comments.
 *
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param   idle_task_stack
 *    Pointer to array for idle task stack.
 *    User must either use the macro `KERNEL_STACK_ARR_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#KERNEL_UWord` with
 *    `#KERNEL_ARCH_STK_ATTR_BEFORE` and `#KERNEL_ARCH_STK_ATTR_AFTER` macros.
 * @param   idle_task_stack_size
 *    Size of idle task stack, in words (`#KERNEL_UWord`)
 * @param   int_stack
 *    Pointer to array for interrupt stack.
 *    User must either use the macro `KERNEL_STACK_ARR_DEF()` for the definition
 *    of stack array, or allocate it manually as an array of `#KERNEL_UWord` with
 *    `#KERNEL_ARCH_STK_ATTR_BEFORE` and `#KERNEL_ARCH_STK_ATTR_AFTER` macros.
 * @param   int_stack_size
 *    Size of interrupt stack, in words (`#KERNEL_UWord`)
 * @param   cb_user_task_create
 *    Callback function that should create initial user's task, see
 *    `#KERNEL_CBUserTaskCreate` for details.
 * @param   cb_idle
 *    Callback function repeatedly called from idle task, see `#KERNEL_CBIdle` for
 *    details.
 */
void kernel_sys_start(
      KERNEL_UWord            *idle_task_stack,
      unsigned int         idle_task_stack_size,
      KERNEL_UWord            *int_stack,
      unsigned int         int_stack_size,
      KERNEL_CBUserTaskCreate *cb_user_task_create,
      KERNEL_CBIdle           *cb_idle
      );

/**
 * Process system tick; should be called periodically, typically
 * from some kind of timer ISR.
 *
 * The period of this timer is determined by user
 * (typically 1 ms, but user is free to set different value)
 *
 * Among other things, expired \ref kernel_timer.h "timers" are fired from this
 * function.
 *
 * For further information, refer to \ref quick_guide "Quick guide".
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
void kernel_tick_int_processing(void);

/**
 * Set time slice ticks value for specified priority (see \ref round_robin).
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param priority
 *    Priority of tasks for which time slice value should be set
 * @param ticks
 *    Time slice value, in ticks. Set to `#KERNEL_NO_TIME_SLICE` for no round-robin
 *    scheduling for given priority (it's default value). Value can't be
 *    higher than `#KERNEL_MAX_TIME_SLICE`.
 *
 * @return
 *    * `#KERNEL_RC_OK` on success;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * `#KERNEL_RC_WPARAM` if given `priority` or `ticks` are invalid.
 */
enum KERNEL_RCode kernel_sys_tslice_set(int priority, int ticks);

/**
 * Get current system ticks count.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return
 *    Current system ticks count.
 */
KERNEL_TickCnt kernel_sys_time_get(void);


/**
 * Set callback function that should be called whenever deadlock occurs or
 * becomes inactive (say, if one of tasks involved in the deadlock was released
 * from wait because of timeout)
 *
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * **Note:** this function should be called from `main()`, before
 * `kernel_sys_start()`.
 *
 * @param cb
 *    Pointer to user-provided callback function.
 *
 * @see `#KERNEL_MUTEX_DEADLOCK_DETECT`
 * @see `#KERNEL_CBDeadlock` for callback function prototype
 */
void kernel_callback_deadlock_set(KERNEL_CBDeadlock *cb);

/**
 * Set callback function that is called when the kernel detects stack overflow
 * (see `#KERNEL_STACK_OVERFLOW_CHECK`).
 *
 * For function prototype, refer to `#KERNEL_CBStackOverflow`.
 */
void kernel_callback_stack_overflow_set(KERNEL_CBStackOverflow *cb);

/**
 * Returns current system state flags
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_StateFlag kernel_sys_state_flags_get(void);

/**
 * Returns system context: task or ISR.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @see `enum #KERNEL_Context`
 */
enum KERNEL_Context kernel_sys_context_get(void);

/**
 * Returns whether current system context is `#KERNEL_CONTEXT_TASK`
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return `KERNEL_TRUE` if current system context is `#KERNEL_CONTEXT_TASK`,
 *         `KERNEL_FALSE` otherwise.
 *
 * @see `kernel_sys_context_get()`
 * @see `enum #KERNEL_Context`
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL kernel_is_task_context(void)
{
   return (kernel_sys_context_get() == KERNEL_CONTEXT_TASK);
}

/**
 * Returns whether current system context is `#KERNEL_CONTEXT_ISR`
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return `KERNEL_TRUE` if current system context is `#KERNEL_CONTEXT_ISR`,
 *         `KERNEL_FALSE` otherwise.
 *
 * @see `kernel_sys_context_get()`
 * @see `enum #KERNEL_Context`
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL kernel_is_isr_context(void)
{
   return (kernel_sys_context_get() == KERNEL_CONTEXT_ISR);
}

/**
 * Returns pointer to the currently running task.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 */
struct KERNEL_Task *kernel_cur_task_get(void);

/**
 * Returns pointer to the body function of the currently running task.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 */
KERNEL_TaskBody *kernel_cur_task_body_get(void);

/**
 * Disable kernel scheduler and return previous scheduler state.
 *
 * Actual behavior depends on the platform:
 *
 * - On Microchip platforms, only scheduler's interrupt gets disabled. All
 *   other interrupts are not affected, independently of their priorities.
 * - On Cortex-M3/M4 platforms, we can only disable interrupts based on
 *   priority. So, this function disables all interrupts with lowest priority
 *   (since scheduler works at lowest interrupt priority).
 * - On Cortex-M0/M0+, we have to disable all interrupts.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @return
 *    State to be restored later by `#kernel_sched_restore()`
 */
_KERNEL_STATIC_INLINE KERNEL_UWord kernel_sched_dis_save(void)
{
   return kernel_arch_sched_dis_save();
}

/**
 * Restore state of the kernel scheduler. See `#kernel_sched_dis_save()`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param sched_state
 *    Value returned from `#kernel_sched_dis_save()`
 */
_KERNEL_STATIC_INLINE void kernel_sched_restore(KERNEL_UWord sched_state)
{
   kernel_arch_sched_restore(sched_state);
}


#if KERNEL_DYNAMIC_TICK || defined(DOXYGEN_ACTIVE)
/**
 * $(KERNEL_IF_ONLY_DYNAMIC_TICK_SET)
 *
 * Set callbacks related to dynamic tick.
 *
 * \attention This function should be called <b>before</b> `kernel_sys_start()`,
 * otherwise, you'll run into run-time error `_KERNEL_FATAL_ERROR()`.
 *
 * $(KERNEL_CALL_FROM_MAIN)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param cb_tick_schedule
 *    Pointer to callback function to schedule next time to call
 *    `kernel_tick_int_processing()`, see `#KERNEL_CBTickSchedule` for the prototype.
 * @param cb_tick_cnt_get
 *    Pointer to callback function to get current system tick counter value,
 *    see `#KERNEL_CBTickCntGet` for the prototype.
 */
void kernel_callback_dyn_tick_set(
      KERNEL_CBTickSchedule   *cb_tick_schedule,
      KERNEL_CBTickCntGet     *cb_tick_cnt_get
      );
#endif


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _KERNEL_SYS_H

