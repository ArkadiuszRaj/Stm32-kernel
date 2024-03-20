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
 * Example of architecture-dependent routines
 */


#ifndef  _KERNEL_ARCH_EXAMPLE_H
#define  _KERNEL_ARCH_EXAMPLE_H

/**
 * FFS - find first set bit. Used in `_find_next_task_to_run()` function.
 * Say, for `0xa8` it should return `3`.
 *
 * May be not defined: in this case, naive algorithm will be used.
 */
#define  _KERNEL_FFS(x) (32 - __builtin_clz((x) & (0 - (x))))

/**
 * Used by the kernel as a signal that something really bad happened.
 * Indicates KERNEL bugs as well as illegal kernel usage, e.g. sleeping in
 * the idle task callback or build-time configuration mismatch (see
 * `#KERNEL_CHECK_BUILD_CFG` for details on the last one)
 *
 * Typically, set to assembler instruction that causes debugger to halt.
 */
#define  _KERNEL_FATAL_ERRORF(error_msg, ...)         \
   {__asm__ volatile(" sdbbp 0"); __asm__ volatile ("nop");}




/**
 * Compiler-specific attribute that should be placed **before** declaration of
 * array used for stack. It is needed because there are often additional
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see KERNEL_ARCH_STK_ATTR_AFTER
 */
#define KERNEL_ARCH_STK_ATTR_BEFORE

/**
 * Compiler-specific attribute that should be placed **after** declaration of
 * array used for stack. It is needed because there are often additional
 * restrictions applied to alignment of stack, so, to meet them, stack arrays
 * need to be declared with these macros.
 *
 * @see KERNEL_ARCH_STK_ATTR_BEFORE
 */

#define KERNEL_ARCH_STK_ATTR_AFTER      __attribute__((aligned(0x8)))

/**
 * Minimum task's stack size, in words, not in bytes; includes a space for
 * context plus for parameters passed to task's body function.
 */
#define  KERNEL_MIN_STACK_SIZE          36

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
 * Value for initializing the unused space of task's stack
 */
#define  KERNEL_FILL_STACK_VAL          0xFEEDFACE




/**
 * Declares variable that is used by macros `KERNEL_INT_DIS_SAVE()` and
 * `KERNEL_INT_RESTORE()` for storing status register value.
 *
 * @see `KERNEL_INT_DIS_SAVE()`
 * @see `KERNEL_INT_RESTORE()`
 */
#define  KERNEL_INTSAVE_DATA            int kernel_save_status_reg = 0;

/**
 * The same as `#KERNEL_INTSAVE_DATA` but for using in ISR together with
 * `KERNEL_INT_IDIS_SAVE()`, `KERNEL_INT_IRESTORE()`.
 *
 * @see `KERNEL_INT_IDIS_SAVE()`
 * @see `KERNEL_INT_IRESTORE()`
 */
#define  KERNEL_INTSAVE_DATA_INT        KERNEL_INTSAVE_DATA

/**
 * Disable interrupts and return previous value of status register,
 * atomically. Similar `kernel_arch_sr_save_int_dis()`, but implemented
 * as a macro, so it is potentially faster.
 *
 * Uses `#KERNEL_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA`
 * @see `kernel_arch_sr_save_int_dis()`
 */
#define KERNEL_INT_DIS_SAVE()        kernel_save_status_reg = kernel_arch_sr_save_int_dis()

/**
 * Restore previously saved status register.
 * Similar to `kernel_arch_sr_restore()`, but implemented as a macro,
 * so it is potentially faster.
 *
 * Uses `#KERNEL_INTSAVE_DATA` as a temporary storage.
 *
 * @see `#KERNEL_INTSAVE_DATA`
 * @see `kernel_arch_sr_save_int_dis()`
 */
#define KERNEL_INT_RESTORE()         kernel_arch_sr_restore(kernel_save_status_reg)

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
#define KERNEL_IS_INT_DISABLED()     ((__builtin_mfc0(12, 0) & 1) == 0)

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

/**
 * If compiler does not conform to c99 standard, there's no inline keyword.
 * So, there's a special macro for that.
 */
#if KERNEL_FORCED_INLINE
//TODO: if available, use some compiler-specific forced inline qualifier
#  define _KERNEL_INLINE             inline
#else
#  define _KERNEL_INLINE             inline
#endif

/**
 * For some compilers, order of these qualifiers matters
 * (at least when _KERNEL_INLINE expands to some compiler-specific forced inline)
 */
#define _KERNEL_STATIC_INLINE        static _KERNEL_INLINE

/**
 * Sometimes compilers are buggy in high-optimization modes, and these
 * bugs are often could be worked around by adding the `volatile` keyword.
 *
 * It is compiler-dependent, so, there's a special macro for that.
 */
#define _KERNEL_VOLATILE_WORKAROUND   /* nothing */


#endif   // _KERNEL_ARCH_EXAMPLE_H
