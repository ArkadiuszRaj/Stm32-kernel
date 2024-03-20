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
 *
 * \file
 *
 * Cortex-M0/M0+/M3/M4/M4F architecture-dependent routines
 *
 */

#ifndef  _KERNEL_ARCH_CORTEX_M_H
#define  _KERNEL_ARCH_CORTEX_M_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "../kernel_arch_detect.h"
#include "../../core/kernel_cfg_dispatch.h"




#ifdef __cplusplus
extern "C"  {     /*}*/
#endif


/*******************************************************************************
 *    ARCH-DEPENDENT DEFINITIONS
 ******************************************************************************/







#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define  _KERNEL_CORTEX_INTSAVE_DATA_INVALID   0xffffffff

#if KERNEL_DEBUG
#  define   _KERNEL_CORTEX_INTSAVE_CHECK()                         \
{                                                              \
   if (KERNEL_INTSAVE_VAR == _KERNEL_CORTEX_INTSAVE_DATA_INVALID){     \
      _KERNEL_FATAL_ERROR("");                                     \
   }                                                           \
}
#else
#  define   _KERNEL_CORTEX_INTSAVE_CHECK()  /* nothing */
#endif

#if defined(__KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__)
/**
 * FFS - find first set bit. Used in `_find_next_task_to_run()` function.
 * Say, for `0xa8` it should return `3`.
 *
 * May be not defined: in this case, naive algorithm will be used.
 */
#define  _KERNEL_FFS(x)     ffs_asm(x)
int ffs_asm(int x);
#endif

/**
 * Used by the kernel as a signal that something really bad happened.
 * Indicates KERNEL bugs as well as illegal kernel usage
 * (e.g. sleeping in the idle task callback)
 *
 * Typically, set to assembler instruction that causes debugger to halt.
 */

#if defined(__KERNEL_COMPILER_IAR__)
#  define  _KERNEL_FATAL_ERRORF(error_msg, ...)         \
      {asm("bkpt #0");}
#else
#  define  _KERNEL_FATAL_ERRORF(error_msg, ...)         \
      {__asm__ volatile("bkpt #0");}
#endif



/**
 * \def KERNEL_ARCH_STK_ATTR_BEFORE
 *
 * Compiler-specific attribute that should be placed **before** declaration of
 * array used for stack. It is needed because there are often additional
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see KERNEL_ARCH_STK_ATTR_AFTER
 */

/**
 * \def KERNEL_ARCH_STK_ATTR_AFTER
 *
 * Compiler-specific attribute that should be placed **after** declaration of
 * array used for stack. It is needed because there are often additional
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see KERNEL_ARCH_STK_ATTR_BEFORE
 */

#if defined(__KERNEL_COMPILER_ARMCC__)

#  define KERNEL_ARCH_STK_ATTR_BEFORE      __align(8)
#  define KERNEL_ARCH_STK_ATTR_AFTER

#elif defined(__KERNEL_COMPILER_GCC__) || defined(__KERNEL_COMPILER_CLANG__)

#  define KERNEL_ARCH_STK_ATTR_BEFORE
#  define KERNEL_ARCH_STK_ATTR_AFTER       __attribute__((aligned(0x08)))

#elif defined(__KERNEL_COMPILER_IAR__)

#  define KERNEL_ARCH_STK_ATTR_BEFORE
#  define KERNEL_ARCH_STK_ATTR_AFTER

#endif


#if defined(__KERNEL_ARCHFEAT_CORTEX_M_FPU__)
#  define _KERNEL_CORTEX_FPU_CONTEXT_SIZE 32 /* FPU registers: S0 .. S31 */
#else
#  define _KERNEL_CORTEX_FPU_CONTEXT_SIZE 0  /* no FPU registers */
#endif


/**
 * Minimum task's stack size, in words, not in bytes; includes a space for
 * context plus for parameters passed to task's body function.
 */
#define  KERNEL_MIN_STACK_SIZE          (17 /* context: 17 words */   \
      + _KERNEL_STACK_OVERFLOW_SIZE_ADD                               \
      + _KERNEL_CORTEX_FPU_CONTEXT_SIZE                               \
      )

/**
 * Width of `int` type.
 */
#define  KERNEL_INT_WIDTH               32

/**
 * Unsigned integer type whose size is equal to the size of CPU register.
 * Typically it's plain `unsigned int`.
 */
typedef  unsigned int               KERNEL_UWord;

/**
 * Unsigned integer type that is able to store pointers.
 * We need it because some platforms don't define `uintptr_t`.
 * Typically it's `unsigned int`.
 */
typedef  unsigned int               KERNEL_UIntPtr;

/**
 * Maximum number of priorities available, this value usually matches
 * `#KERNEL_INT_WIDTH`.
 *
 * @see KERNEL_PRIORITIES_CNT
 */
#define  KERNEL_PRIORITIES_MAX_CNT      KERNEL_INT_WIDTH

/**
 * Value for infinite waiting, usually matches `ULONG_MAX`,
 * because `#KERNEL_TickCnt` is declared as `unsigned long`.
 */
#define  KERNEL_WAIT_INFINITE           (KERNEL_TickCnt)0xFFFFFFFF

/**
 * Value for initializing the task's stack
 */
#define  KERNEL_FILL_STACK_VAL          0xFEEDFACE




/**
 * Variable name that is used for storing interrupts state
 * by macros KERNEL_INTSAVE_DATA and friends
 */
#define KERNEL_INTSAVE_VAR              kernel_save_status_reg

/**
 * Declares variable that is used by macros `KERNEL_INT_DIS_SAVE()` and
 * `KERNEL_INT_RESTORE()` for storing status register value.
 *
 * It is good idea to initially set it to some invalid value,
 * and if KERNEL_DEBUG is non-zero, check it in KERNEL_INT_RESTORE().
 * Then, we can catch bugs if someone tries to restore interrupts status
 * without saving it first.
 *
 * @see `KERNEL_INT_DIS_SAVE()`
 * @see `KERNEL_INT_RESTORE()`
 */
#define  KERNEL_INTSAVE_DATA            \
   KERNEL_UWord KERNEL_INTSAVE_VAR = _KERNEL_CORTEX_INTSAVE_DATA_INVALID;

/**
 * The same as `#KERNEL_INTSAVE_DATA` but for using in ISR together with
 * `KERNEL_INT_IDIS_SAVE()`, `KERNEL_INT_IRESTORE()`.
 *
 * @see `KERNEL_INT_IDIS_SAVE()`
 * @see `KERNEL_INT_IRESTORE()`
 */
#define  KERNEL_INTSAVE_DATA_INT        KERNEL_INTSAVE_DATA

/**
 * \def KERNEL_INT_DIS_SAVE()
 *
 * Disable interrupts and return previous value of status register,
 * atomically. Similar `kernel_arch_sr_save_int_dis()`, but implemented
 * as a macro, so it is potentially faster.
 *
 * Uses `#KERNEL_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA`
 * @see `kernel_arch_sr_save_int_dis()`
 */

/**
 * \def KERNEL_INT_RESTORE()
 *
 * Restore previously saved status register.
 * Similar to `kernel_arch_sr_restore()`, but implemented as a macro,
 * so it is potentially faster.
 *
 * Uses `#KERNEL_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA`
 * @see `kernel_arch_sr_save_int_dis()`
 */

#define KERNEL_INT_DIS_SAVE()   KERNEL_INTSAVE_VAR = kernel_arch_sr_save_int_dis()
#define KERNEL_INT_RESTORE()    _KERNEL_CORTEX_INTSAVE_CHECK();                     \
                            kernel_arch_sr_restore(KERNEL_INTSAVE_VAR)

/**
 * The same as `KERNEL_INT_DIS_SAVE()` but for using in ISR.
 *
 * Uses `#KERNEL_INTSAVE_DATA_INT` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA_INT`
 */
#define KERNEL_INT_IDIS_SAVE()       KERNEL_INT_DIS_SAVE()

/**
 * The same as `KERNEL_INT_RESTORE()` but for using in ISR.
 *
 * Uses `#KERNEL_INTSAVE_DATA_INT` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA_INT`
 */
#define KERNEL_INT_IRESTORE()        KERNEL_INT_RESTORE()

/**
 * Returns nonzero if interrupts are disabled, zero otherwise.
 */
#define KERNEL_IS_INT_DISABLED()     (_kernel_arch_is_int_disabled())

/**
 * Pend context switch from interrupt.
 */
#define _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED()          \
   _kernel_context_switch_pend_if_needed()

/**
 * Converts size in bytes to size in `#KERNEL_UWord`.
 * For 32-bit platforms, we should shift it by 2 bit to the right;
 * for 16-bit platforms, we should shift it by 1 bit to the right.
 */
#define _KERNEL_SIZE_BYTES_TO_UWORDS(size_in_bytes)    ((size_in_bytes) >> 2)

#if defined(__KERNEL_COMPILER_ARMCC__)
#  if KERNEL_FORCED_INLINE
#     define _KERNEL_INLINE             __forceinline
#  else
#     define _KERNEL_INLINE             __inline
#  endif
#  define _KERNEL_STATIC_INLINE         static _KERNEL_INLINE
#  define _KERNEL_VOLATILE_WORKAROUND   /* nothing */
#elif defined(__KERNEL_COMPILER_GCC__) || defined(__KERNEL_COMPILER_CLANG__)
#  if KERNEL_FORCED_INLINE
#     define _KERNEL_INLINE             inline __attribute__ ((always_inline))
#  else
#     define _KERNEL_INLINE             inline
#  endif
#  define _KERNEL_STATIC_INLINE         static _KERNEL_INLINE
#  define _KERNEL_VOLATILE_WORKAROUND   /* nothing */
#elif defined(__KERNEL_COMPILER_IAR__)
#  if KERNEL_FORCED_INLINE
#     define _KERNEL_INLINE             _Pragma("inline=forced")
#  else
#     define _KERNEL_INLINE             inline
#  endif
/*
 * NOTE: for IAR, `_KERNEL_INLINE` should go before `static`, because
 * when we use forced inline by _Pragma, and `static` is before _Pragma,
 * then IAR compiler generates a warning that pragma should immediately
 * precede the declaration.
 */
#  define _KERNEL_STATIC_INLINE         _KERNEL_INLINE static
#  define _KERNEL_VOLATILE_WORKAROUND   volatile
#else
#  error unknown Cortex compiler
#endif

#define _KERNEL_ARCH_STACK_PT_TYPE   _KERNEL_ARCH_STACK_PT_TYPE__FULL
#define _KERNEL_ARCH_STACK_DIR       _KERNEL_ARCH_STACK_DIR__DESC

#endif   //-- DOXYGEN_SHOULD_SKIP_THIS











#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif   // _KERNEL_ARCH_CORTEX_M_H

