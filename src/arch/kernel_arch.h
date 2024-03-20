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
 * Architecture-dependent routines declaration.
 */

#ifndef _KERNEL_ARCH_H
#define _KERNEL_ARCH_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../core/kernel_common.h"



/*******************************************************************************
 *    OPTION VALUES
 ******************************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define _KERNEL_ARCH_STACK_DIR__ASC        1
#define _KERNEL_ARCH_STACK_DIR__DESC       2

//-- Note: the macro _KERNEL_ARCH_STACK_DIR is defined in the header for each
//   particular achitecture


#define _KERNEL_ARCH_STACK_PT_TYPE__FULL   3
#define _KERNEL_ARCH_STACK_PT_TYPE__EMPTY  4

//-- Note: the macro _KERNEL_ARCH_STACK_PT_TYPE is defined in the header for each
//   particular achitecture



#define _KERNEL_ARCH_STACK_IMPL__FULL_ASC        5
#define _KERNEL_ARCH_STACK_IMPL__FULL_DESC       6
#define _KERNEL_ARCH_STACK_IMPL__EMPTY_ASC       7
#define _KERNEL_ARCH_STACK_IMPL__EMPTY_DESC      8

//-- Note: the macro _KERNEL_ARCH_STACK_IMPL is defined below in this file


#endif





/*******************************************************************************
 *    ACTUAL PORT IMPLEMENTATION
 ******************************************************************************/

#if defined(__KERNEL_ARCH_PIC32MX__)
#  include "pic32/kernel_arch_pic32.h"
#elif defined(__KERNEL_ARCH_PIC24_DSPIC__)
#  include "pic24_dspic/kernel_arch_pic24.h"
#elif defined(__KERNEL_ARCH_CORTEX_M__)
#  include "cortex_m/kernel_arch_cortex_m.h"
#else
#  error "unknown platform"
#endif




//-- Now, define _KERNEL_ARCH_STACK_IMPL depending on _KERNEL_ARCH_STACK_DIR and
//   _KERNEL_ARCH_STACK_PT_TYPE

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#if !defined(_KERNEL_ARCH_STACK_DIR)
#  error _KERNEL_ARCH_STACK_DIR is not defined
#endif

#if !defined(_KERNEL_ARCH_STACK_PT_TYPE)
#  error _KERNEL_ARCH_STACK_PT_TYPE is not defined
#endif

#if (_KERNEL_ARCH_STACK_DIR == _KERNEL_ARCH_STACK_DIR__ASC)
#  if (_KERNEL_ARCH_STACK_PT_TYPE == _KERNEL_ARCH_STACK_PT_TYPE__FULL)
#     define _KERNEL_ARCH_STACK_IMPL    _KERNEL_ARCH_STACK_IMPL__FULL_ASC
#  elif (_KERNEL_ARCH_STACK_PT_TYPE == _KERNEL_ARCH_STACK_PT_TYPE__EMPTY)
#     define _KERNEL_ARCH_STACK_IMPL    _KERNEL_ARCH_STACK_IMPL__EMPTY_ASC
#  else
#     error wrong _KERNEL_ARCH_STACK_PT_TYPE
#  endif
#elif (_KERNEL_ARCH_STACK_DIR == _KERNEL_ARCH_STACK_DIR__DESC)
#  if (_KERNEL_ARCH_STACK_PT_TYPE == _KERNEL_ARCH_STACK_PT_TYPE__FULL)
#     define _KERNEL_ARCH_STACK_IMPL    _KERNEL_ARCH_STACK_IMPL__FULL_DESC
#  elif (_KERNEL_ARCH_STACK_PT_TYPE == _KERNEL_ARCH_STACK_PT_TYPE__EMPTY)
#     define _KERNEL_ARCH_STACK_IMPL    _KERNEL_ARCH_STACK_IMPL__EMPTY_DESC
#  else
#     error wrong _KERNEL_ARCH_STACK_PT_TYPE
#  endif
#else
#  error wrong _KERNEL_ARCH_STACK_DIR
#endif

#endif



#ifdef __cplusplus
extern "C"  {  /*}*/
#endif

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


/**
 * Unconditionally disable <i>system interrupts</i>.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
void kernel_arch_int_dis(void);

/**
 * Unconditionally enable interrupts
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
void kernel_arch_int_en(void);

/**
 * Disable <i>system interrupts</i> and return previous value of status
 * register, atomically.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 *
 * @see `kernel_arch_sr_restore()`
 */
KERNEL_UWord kernel_arch_sr_save_int_dis(void);

/**
 * Restore previously saved status register.
 *
 * @param sr   status register value previously from
 *             `kernel_arch_sr_save_int_dis()`
 *
 * @see `kernel_arch_sr_save_int_dis()`
 */
void kernel_arch_sr_restore(KERNEL_UWord sr);

/**
 * Disable kernel scheduler and return previous state.
 *
 * @return
 *    Scheduler state to be restored later by `#kernel_arch_sched_restore()`.
 */
KERNEL_UWord kernel_arch_sched_dis_save(void);

/**
 * Restore state of the kernel scheduler. See `#kernel_arch_sched_dis_save()`.
 *
 * @param sched_state
 *    Value returned from `#kernel_arch_sched_dis_save()`
 */
void kernel_arch_sched_restore(KERNEL_UWord sched_state);

/**
 * Should put initial CPU context to the provided stack pointer for new task
 * and return current stack pointer.
 *
 * When resulting context gets restored by
 * `_kernel_arch_context_switch_now_nosave()` or `_kernel_arch_context_switch_pend()`,
 * the following conditions should be met:
 *
 * - Interrupts are enabled;
 * - Return address is set to `kernel_task_exit()`, so that when task body function
 *   returns, `kernel_task_exit()` gets automatially called;
 * - Argument 0 contains `param` pointer
 *
 * @param task_func
 *    Pointer to task body function.
 * @param stack_low_addr
 *    Lowest address of the stack, independently of the architecture stack
 *    implementation
 * @param stack_high_addr
 *    Highest address of the stack, independently of the architecture stack
 *    implementation
 * @param param
 *    User-provided parameter for task body function.
 *
 * @return current stack pointer (top of the stack)
 */
KERNEL_UWord *_kernel_arch_stack_init(
      KERNEL_TaskBody   *task_func,
      KERNEL_UWord      *stack_low_addr,
      KERNEL_UWord      *stack_high_addr,
      void          *param
      );



/**
 * Should return 1 if <i>system ISR</i> is currently running, 0 otherwise.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * ISR</i>.
 */
int _kernel_arch_inside_isr(void);

/**
 * Should return 1 if <i>system interrupts</i> are currently disabled, 0
 * otherwise.
 *
 * Refer to the section \ref interrupt_types for details on what is <i>system
 * interrupt</i>.
 */
int _kernel_arch_is_int_disabled(void);

/**
 * Called whenever we need to switch context from one task to another.
 *
 * This function typically does NOT switch context; it merely pends it,
 * that is, it sets appropriate interrupt flag. If current level is an
 * application level, interrupt is fired immediately, and context gets
 * switched. Otherwise (if some ISR is currently running), context switch
 * keeps pending until all ISR return.
 *
 * **Preconditions:**
 *
 * - interrupts are enabled;
 * - `_kernel_curr_run_task` points to currently running (preempted) task;
 * - `_kernel_next_task_to_run` points to new task to run.
 *
 * **Actions to perform in actual context switching routine:**
 *
 * - save context of the preempted task to its stack;
 * - if preprocessor macro `#_KERNEL_ON_CONTEXT_SWITCH_HANDLER` is non-zero, call
 *   `_kernel_sys_on_context_switch(_kernel_curr_run_task, _kernel_next_task_to_run);`.
 * - set `_kernel_curr_run_task` to `_kernel_next_task_to_run`;
 * - restore context of the newly activated task from its stack.
 *
 * @see `_kernel_curr_run_task`
 * @see `_kernel_next_task_to_run`
 */
void _kernel_arch_context_switch_pend(void);

/**
 * Called whenever we need to switch context to new task, but don't save
 * current context. This happens:
 * - At system start, inside `kernel_sys_start()`
 *   (well, it is actually called indirectly but from `_kernel_arch_sys_start()`);
 * - At task exit, inside `kernel_task_exit()`.
 *
 * This function doesn't need to pend context switch, it switches context
 * immediately.
 *
 * **Preconditions:**
 *
 * - interrupts are disabled;
 * - `_kernel_next_task_to_run` is already set to needed task.
 *
 * **Actions to perform:**
 *
 * - if preprocessor macro `#_KERNEL_ON_CONTEXT_SWITCH_HANDLER` is non-zero, call
 *   `_kernel_sys_on_context_switch(_kernel_curr_run_task, _kernel_next_task_to_run);`.
 * - set `_kernel_curr_run_task` to `_kernel_next_task_to_run`;
 * - restore context of the newly activated task from its stack.
 *
 * @see `_kernel_curr_run_task`
 * @see `_kernel_next_task_to_run`
 */
void _kernel_arch_context_switch_now_nosave(void);

/**
 * Performs first context switch to the first task (`_kernel_next_task_to_run` is
 * already set to needed task).
 *
 * Typically, this function just calls `_kernel_arch_context_switch_now_nosave()`,
 * but it also can perform any architecture-dependent actions first, if needed.
 */
void _kernel_arch_sys_start(
      KERNEL_UWord      *int_stack,
      KERNEL_UWord       int_stack_size
      );


#ifdef __cplusplus
}  /* extern "C" */
#endif









#endif  /* _KERNEL_ARCH_H */

