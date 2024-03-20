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
 * Definitions used through the whole kernel.
 */

#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../arch/kernel_arch_detect.h"
#include "kernel_cfg_dispatch.h"

#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Magic number for object validity verification
 */
// TODO: use KERNEL_UWord here instead of unsigned int
enum KERNEL_ObjId {
   KERNEL_ID_NONE           = (int)0x0,         //!< id for invalid object
   KERNEL_ID_TASK           = (int)0x47ABCF69,  //!< id for tasks
   KERNEL_ID_SEMAPHORE      = (int)0x6FA173EB,  //!< id for semaphores
   KERNEL_ID_EVENTGRP       = (int)0x5E224F25,  //!< id for event groups
   KERNEL_ID_DATAQUEUE      = (int)0x0C8A6C89,  //!< id for data queues
   KERNEL_ID_FSMEMORYPOOL   = (int)0x26B7CE8B,  //!< id for fixed memory pools
   KERNEL_ID_MUTEX          = (int)0x17129E45,  //!< id for mutexes
   KERNEL_ID_TIMER          = (int)0x1A937FBC,  //!< id for timers
   KERNEL_ID_EXCHANGE       = (int)0x32b7c072,  //!< id for exchange objects
   KERNEL_ID_EXCHANGE_LINK  = (int)0x24d36f35,  //!< id for exchange link
};

/**
 * Result code returned by kernel services.
 */
enum KERNEL_RCode {
   ///
   /// Successful operation
   KERNEL_RC_OK                   =   0,
   ///
   /// Timeout (consult `#KERNEL_TickCnt` for details).
   /// @see `#KERNEL_TickCnt`
   KERNEL_RC_TIMEOUT              =  -1,
   ///
   /// This code is returned in the following cases:
   ///   * Trying to increment semaphore count more than its max count;
   ///   * Trying to return extra memory block to fixed memory pool.
   /// @see kernel_sem.h
   /// @see kernel_fmem.h
   KERNEL_RC_OVERFLOW             =  -2,
   ///
   /// Wrong context error: returned if function is called from
   /// non-acceptable context. Required context suggested for every
   /// function by badges:
   ///
   ///   * $(KERNEL_CALL_FROM_TASK) - function can be called from task;
   ///   * $(KERNEL_CALL_FROM_ISR) - function can be called from ISR.
   ///
   /// @see `kernel_sys_context_get()`
   /// @see `enum #KERNEL_Context`
   KERNEL_RC_WCONTEXT             =  -3,
   ///
   /// Wrong task state error: requested operation requires different
   /// task state
   KERNEL_RC_WSTATE               =  -4,
   ///
   /// This code is returned by most of the kernel functions when
   /// wrong params were given to function. This error code can be returned
   /// if only build-time option `#KERNEL_CHECK_PARAM` is non-zero
   /// @see `#KERNEL_CHECK_PARAM`
   KERNEL_RC_WPARAM               =  -5,
   ///
   /// Illegal usage. Returned in the following cases:
   /// * task tries to unlock or delete the mutex that is locked by different
   ///   task,
   /// * task tries to lock mutex with priority ceiling whose priority is
   ///   lower than task's priority
   /// @see kernel_mutex.h
   KERNEL_RC_ILLEGAL_USE          =  -6,
   ///
   /// Returned when user tries to perform some operation on invalid object
   /// (mutex, semaphore, etc).
   /// Object validity is checked by comparing special `id_...` field value
   /// with the value from `enum #KERNEL_ObjId`
   /// @see `#KERNEL_CHECK_PARAM`
   KERNEL_RC_INVALID_OBJ          =  -7,
   /// Object for whose event task was waiting is deleted.
   KERNEL_RC_DELETED              =  -8,
   /// Task was released from waiting forcibly because some other task
   /// called `kernel_task_release_wait()`
   KERNEL_RC_FORCED               =  -9,
   /// Internal kernel error, should never be returned by kernel services.
   /// If it is returned, it's a bug in the kernel.
   KERNEL_RC_INTERNAL             = -10,
};

/**
 * Prototype for task body function.
 */
typedef void (KERNEL_TaskBody)(void *param);


/**
 * Type for system tick count, it is used by the kernel to represent absolute
 * tick count value as well as relative timeouts.
 *
 * When it is used as a timeout value, it represents the maximum number of
 * system ticks to wait.
 *
 * Assume user called some system function, and it can't perform its job
 * immediately (say, it needs to lock mutex but it is already locked, etc).
 *
 * So, function can wait or return an error. There are possible `timeout`
 * values and appropriate behavior of the function:
 *
 * - `timeout` is set to `0`: function doesn't wait at all, no context switch
 *   is performed, `#KERNEL_RC_TIMEOUT` is returned immediately.
 * - `timeout` is set to `#KERNEL_WAIT_INFINITE`: function waits until it
 *   eventually **can** perform its job. Timeout is not taken in account, so
 *   `#KERNEL_RC_TIMEOUT` is never returned.
 * - `timeout` is set to other value: function waits at most specified number
 *   of system ticks.  Strictly speaking, it waits from `(timeout - 1)` to
 *   `timeout` ticks. So, if you specify that timeout is 1, be aware that it
 *   might actually don't wait at all: if $(KERNEL_SYS_TIMER_LINK) interrupt
 *   happens just while function is putting task to wait (with interrupts
 *   disabled), then ISR will be executed right after function puts task to
 *   wait. Then `kernel_tick_int_processing()` will immediately remove the task
 *   from wait queue and make it runnable again.
 *
 *      So, to guarantee that task waits *at least* 1 system tick, you should
 *      specify timeout value of `2`.
 *
 * **Note** also that there are other possible ways to make task runnable:
 *
 * - if task waits because of call to `kernel_task_sleep()`, it may be woken up by
 *   some other task, by means of `kernel_task_wakeup()`. In this case,
 *   `kernel_task_sleep()` returns `#KERNEL_RC_OK`.
 * - independently of the wait reason, task may be released from wait forcibly,
 *   by means of `kernel_task_release_wait()`. It this case, `#KERNEL_RC_FORCED` is
 *   returned by the waiting function.  (the usage of the
 *   `kernel_task_release_wait()` function is discouraged though)
 */
typedef unsigned long KERNEL_TickCnt;

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/// NULL pointer definition
#ifndef KERNEL_NULL
#  ifdef __cplusplus
#     define KERNEL_NULL    0
#  else
#     define KERNEL_NULL    ((void *)0)
#  endif
#endif

/// boolean type definition
#ifndef KERNEL_BOOL
#  ifdef __cplusplus
#     define KERNEL_BOOL    bool
#  else
#     define KERNEL_BOOL    int
#  endif
#endif

/// `true` value definition for type `#KERNEL_BOOL`
#ifndef KERNEL_TRUE
#  define KERNEL_TRUE       (1 == 1)
#endif

/// `false` value definition for type `#KERNEL_BOOL`
#ifndef KERNEL_FALSE
#  define KERNEL_FALSE      (1 == 0)
#endif

/**
 * Macro for making a number a multiple of `sizeof(#KERNEL_UWord)`, should be used
 * with fixed memory block pool. See `kernel_fmem_create()` for usage example.
 */
#define  KERNEL_MAKE_ALIG_SIZE(a)  \
   (((a) + (sizeof(KERNEL_UWord) - 1)) & (~(sizeof(KERNEL_UWord) - 1)))

//-- self-checking
#if (!defined KERNEL_API_MAKE_ALIG_ARG)
#  error KERNEL_API_MAKE_ALIG_ARG is not defined
#elif (!defined KERNEL_API_MAKE_ALIG_ARG__TYPE)
#  error KERNEL_API_MAKE_ALIG_ARG__TYPE is not defined
#elif (!defined KERNEL_API_MAKE_ALIG_ARG__SIZE)
#  error KERNEL_API_MAKE_ALIG_ARG__SIZE is not defined
#endif

//-- define MAKE_ALIG accordingly to config
/**
 * The same as `#KERNEL_MAKE_ALIG_SIZE` but its behavior depends on the option
 * `#KERNEL_API_MAKE_ALIG_ARG`
 *
 * \attention it is recommended to use `#KERNEL_MAKE_ALIG_SIZE` macro instead
 * of this one, in order to avoid confusion caused by various
 * KERNELKernel ports: refer to the section \ref kernelkernel_diff_make_alig for details.
 */
#if (KERNEL_API_MAKE_ALIG_ARG == KERNEL_API_MAKE_ALIG_ARG__TYPE)
#  define  KERNEL_MAKE_ALIG(a)  KERNEL_MAKE_ALIG_SIZE(sizeof(a))
#elif (KERNEL_API_MAKE_ALIG_ARG == KERNEL_API_MAKE_ALIG_ARG__SIZE)
#  define  KERNEL_MAKE_ALIG(a)  KERNEL_MAKE_ALIG_SIZE(a)
#else
#  error wrong KERNEL_API_MAKE_ALIG_ARG
#endif

/**
 * Suppresses "unused" compiler warning for some particular symbol
 */
#define _KERNEL_UNUSED(x) (void)(x)

#define _KERNEL_FATAL_ERROR(error_msg) _KERNEL_FATAL_ERRORF(error_msg, NULL)

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_COMMON_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


