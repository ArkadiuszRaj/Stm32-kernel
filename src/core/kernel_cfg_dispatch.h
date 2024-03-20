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
 * Dispatch configuration: set predefined options, include user-provided cfg
 * file as well as default cfg file.
 *
 */

#ifndef _KERNEL_CFG_DISPATCH_H
#define _KERNEL_CFG_DISPATCH_H

#include "../arch/kernel_arch_detect.h"

//-- Configuration constants
/**
 * In this case, you should use macro like this: `#KERNEL_MAKE_ALIG(struct my_struct)`.
 * This way is used in the majority of KERNELKernel ports. (actually, in all ports
 * except the one by AlexB)
 */
#define  KERNEL_API_MAKE_ALIG_ARG__TYPE       1

/**
 * In this case, you should use macro like this: `#KERNEL_MAKE_ALIG(sizeof(struct
 * my_struct))`. This way is stated in KERNELKernel docs and used in the port for
 * dsPIC/PIC24/PIC32 by AlexB.
 */
#define  KERNEL_API_MAKE_ALIG_ARG__SIZE       2


//--- As a starting point, you might want to copy kernel_cfg_default.h -> kernel_cfg.h,
//    and then edit it if you want to change default configuration.
//    NOTE: the file kernel_cfg.h is specified in .hgignore file, in order to not include
//    project-specific configuration in the common KERNELKernel repository.
#include "kernel_cfg.h"

//-- Probably a bit of hack, but anyway:
//   kernel_cfg.h might just be a modified copy from existing kernel_cfg_default.h,
//   and then, _KERNEL_CFG_DEFAULT_H is probably defined there.
//   But we need to set some defaults, so, let's undef it.
//   Anyway, kernel_cfg_default.h checks whether each particular option is already
//   defined, so it works nice.
#undef _KERNEL_CFG_DEFAULT_H

//--- default cfg file is included too, so that you are free to not set
//    all available options in your kernel_cfg.h file.
#include "kernel_cfg_default.h"

//-- check that all options specified {{{

#if !defined(KERNEL_CHECK_BUILD_CFG)
#  error KERNEL_CHECK_BUILD_CFG is not defined
#endif

#if !defined(KERNEL_PRIORITIES_CNT)
#  error KERNEL_PRIORITIES_CNT is not defined
#endif


#if !defined(KERNEL_CHECK_PARAM)
#  error KERNEL_CHECK_PARAM is not defined
#endif

#if !defined(KERNEL_DEBUG)
#  error KERNEL_DEBUG is not defined
#endif

#if !defined(KERNEL_OLD_KERNELKERNEL_NAMES)
#  error KERNEL_OLD_KERNELKERNEL_NAMES is not defined
#endif

#if !defined(KERNEL_USE_MUTEXES)
#  error KERNEL_USE_MUTEXES is not defined
#endif

#if KERNEL_USE_MUTEXES
#  if !defined(KERNEL_MUTEX_REC)
#     error KERNEL_MUTEX_REC is not defined
#  endif
#  if !defined(KERNEL_MUTEX_DEADLOCK_DETECT)
#     error KERNEL_MUTEX_DEADLOCK_DETECT is not defined
#  endif
#endif

#if !defined(KERNEL_TICK_LISTS_CNT)
#  error KERNEL_TICK_LISTS_CNT is not defined
#endif

#if !defined(KERNEL_API_MAKE_ALIG_ARG)
#  error KERNEL_API_MAKE_ALIG_ARG is not defined
#endif

#if !defined(KERNEL_PROFILER)
#  error KERNEL_PROFILER is not defined
#endif

#if !defined(KERNEL_PROFILER_WAIT_TIME)
#  error KERNEL_PROFILER_WAIT_TIME is not defined
#endif

#if !defined(KERNEL_INIT_INTERRUPT_STACK_SPACE)
#  error KERNEL_INIT_INTERRUPT_STACK_SPACE is not defined
#endif

#if !defined(KERNEL_STACK_OVERFLOW_CHECK)
#  error KERNEL_STACK_OVERFLOW_CHECK is not defined
#endif

#if defined (__KERNEL_ARCH_PIC24_DSPIC__)
#  if !defined(KERNEL_P24_SYS_IPL)
#     error KERNEL_P24_SYS_IPL is not defined
#  endif
#endif

#if !defined(KERNEL_DYNAMIC_TICK)
#  error KERNEL_DYNAMIC_TICK is not defined
#endif

#if !defined(KERNEL_OLD_EVENT_API)
#  error KERNEL_OLD_EVENT_API is not defined
#endif

#if !defined(KERNEL_FORCED_INLINE)
#  error KERNEL_FORCED_INLINE is not defined
#endif

#if !defined(KERNEL_MAX_INLINE)
#  error KERNEL_MAX_INLINE is not defined
#endif


// }}}



//-- check KERNEL_P24_SYS_IPL: should be 1 .. 6.
#if defined (__KERNEL_ARCH_PIC24_DSPIC__)
#  if KERNEL_P24_SYS_IPL >= 7
#     error KERNEL_P24_SYS_IPL must be less than 7
#  endif
#  if KERNEL_P24_SYS_IPL <= 0
#     error KERNEL_P24_SYS_IPL must be more than 0
#  endif
#endif

//-- NOTE: KERNEL_TICK_LISTS_CNT is checked in kernel_timer_static.c
//-- NOTE: KERNEL_PRIORITIES_CNT is checked in kernel_sys.c
//-- NOTE: KERNEL_API_MAKE_ALIG_ARG is checked in kernel_common.h


/**
 * Internal kernel definition: set to non-zero if `_kernel_sys_on_context_switch()`
 * should be called on context switch.
 */
#if KERNEL_PROFILER || KERNEL_STACK_OVERFLOW_CHECK
#  define   _KERNEL_ON_CONTEXT_SWITCH_HANDLER  1
#else
#  define   _KERNEL_ON_CONTEXT_SWITCH_HANDLER  0
#endif

/**
 * If `#KERNEL_STACK_OVERFLOW_CHECK` is set, we have 1-word overhead for each
 * task stack.
 */
#define _KERNEL_STACK_OVERFLOW_SIZE_ADD    (KERNEL_STACK_OVERFLOW_CHECK ? 1 : 0)

#endif // _KERNEL_CFG_DISPATCH_H


