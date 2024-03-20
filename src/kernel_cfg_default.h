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
 * KERNEL default configuration file, to be copied as `kernel_cfg.h`.
 *
 * This project is intended to be built as a library, separately from main
 * project (although nothing prevents you from bundling things together, if you
 * want to).
 *
 * There are various options available which affects API and behavior of the
 * kernel. But these options are specific for particular project, and aren't
 * related to the kernel itself, so we need to keep them separately.
 *
 * To this end, file `kernel.h` (the main kernel header file) includes `kernel_cfg.h`,
 * which isn't included in the repository (even more, it is added to
 * `.hgignore` list actually). Instead, default configuration file
 * `kernel_cfg_default.h` is provided, and when you just cloned the repository, you
 * might want to copy it as `kernel_cfg.h`. Or even better, if your filesystem
 * supports symbolic links, copy it somewhere to your main project's directory
 * (so that you can add it to your VCS there), and create symlink to it named
 * `kernel_cfg.h` in the KERNEL source directory, like this:
 *
 *     $ cd /path/to/kernel/src
 *     $ cp ./kernel_cfg_default.h /path/to/main/project/lib_cfg/kernel_cfg.h
 *     $ ln -s /path/to/main/project/lib_cfg/kernel_cfg.h ./kernel_cfg.h
 *
 * Default configuration file contains detailed comments, so you can read them
 * and configure behavior as you like.
 */

#ifndef _KERNEL_CFG_DEFAULT_H
#define _KERNEL_CFG_DEFAULT_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- some defaults depend on architecture, so we should include
//   `kernel_arch_detect.h`
#include "arch/kernel_arch_detect.h"


/*******************************************************************************
 *    USER-DEFINED OPTIONS
 ******************************************************************************/

/**
 * This option enables run-time check which ensures that build-time options for
 * the kernel match ones for the application.
 *
 * Without this check, it is possible that you change your `kernel_cfg.h` file, and
 * just rebuild your application without rebuilding the kernel. Then,
 * application would assume that kernel behaves accordingly to `kernel_cfg.h` which
 * was included in the application, but this is actually not true: you need to
 * rebuild the kernel for changes to take effect.
 *
 * With this option turned on, if build-time configurations don't match, you
 * will get run-time error (`_KERNEL_FATAL_ERROR()`) inside `kernel_sys_start()`, which
 * is much more informative than weird bugs caused by configuration mismatch.
 *
 * <b>Note</b>: turning this option on makes sense if only you use KERNEL
 * as a separate library. If you build KERNEL together with the
 * application, both the kernel and the application always use the same
 * `kernel_cfg.h` file, therefore this option is useless.
 *
 * \attention If this option is on, your application must include the file
 * `kernel_app_check.c`.
 */
#ifndef KERNEL_CHECK_BUILD_CFG
#  define KERNEL_CHECK_BUILD_CFG        1
#endif


/**
 * Number of priorities that can be used by application, plus one for idle task
 * (which has the lowest priority). This value can't be higher than
 * architecture-dependent value `#KERNEL_PRIORITIES_MAX_CNT`, which typically
 * equals to width of `int` type. So, for 32-bit systems, max number of
 * priorities is 32.
 *
 * But usually, application needs much less: I can imagine **at most** 4-5
 * different priorities, plus one for the idle task.
 *
 * Do note also that each possible priority level takes RAM: two pointers for
 * linked list and one `short` for time slice value, so on 32-bit system it
 * takes 10 bytes. So, with default value of 32 priorities available, it takes
 * 320 bytes. If you set it, say, to 5, you save `270` bytes, which might be
 * notable.
 *
 * Default: `#KERNEL_PRIORITIES_MAX_CNT`.
 */
#ifndef KERNEL_PRIORITIES_CNT
#  define KERNEL_PRIORITIES_CNT      KERNEL_PRIORITIES_MAX_CNT
#endif

/**
 * Enables additional param checking for most of the system functions.
 * It's surely useful for debug, but probably better to remove in release.
 * If it is set, most of the system functions are able to return two additional
 * codes:
 *
 *    * `#KERNEL_RC_WPARAM` if wrong params were given;
 *    * `#KERNEL_RC_INVALID_OBJ` if given pointer doesn't point to a valid object.
 *      Object validity is checked by means of the special ID field of type
 *      `enum #KERNEL_ObjId`.
 *
 * @see `enum #KERNEL_ObjId`
 */
#ifndef KERNEL_CHECK_PARAM
#  define KERNEL_CHECK_PARAM         1
#endif

/**
 * Allows additional internal self-checking, useful to catch internal
 * KERNEL bugs as well as illegal kernel usage (e.g. sleeping in the idle
 * task callback). Produces a couple of extra instructions which usually just
 * causes debugger to stop if something goes wrong.
 */
#ifndef KERNEL_DEBUG
#  define KERNEL_DEBUG               0
#endif

/**
 * Whether old KERNELKernel names (definitions, functions, etc) should be
 * available.  If you're porting your existing application written for
 * KERNELKernel, it is definitely worth enabling.  If you start new project with
 * KERNEL from scratch, it's better to avoid old names.
 */
#ifndef KERNEL_OLD_KERNELKERNEL_NAMES
#  define KERNEL_OLD_KERNELKERNEL_NAMES  1
#endif

/**
 * Whether mutexes API should be available
 */
#ifndef KERNEL_USE_MUTEXES
#  define KERNEL_USE_MUTEXES         1
#endif

/**
 * Whether mutexes should allow recursive locking/unlocking
 */
#ifndef KERNEL_MUTEX_REC
#  define KERNEL_MUTEX_REC           1
#endif

/**
 * Whether RTOS should detect deadlocks and notify user about them
 * via callback
 *
 * @see see `kernel_callback_deadlock_set()`
 * @see see `#KERNEL_CBDeadlock`
 */
#ifndef KERNEL_MUTEX_DEADLOCK_DETECT
#  define KERNEL_MUTEX_DEADLOCK_DETECT  1
#endif

/**
 *
 * <i>Takes effect if only `#KERNEL_DYNAMIC_TICK` is <B>not set</B></i>.
 *
 * Number of "tick" lists of timers, must be a power or two; minimum value:
 * `2`; typical values: `4`, `8` or `16`.
 *
 * Refer to the \ref timers_static_implementation for details.
 *
 * Shortly: this value represents number of elements in the array of
 * `struct KERNEL_ListItem`, on 32-bit system each element takes 8 bytes.
 *
 * The larger value, the more memory is needed, and the faster
 * $(KERNEL_SYS_TIMER_LINK) ISR works. If your application has a lot of timers
 * and/or sleeping tasks, consider incrementing this value; otherwise,
 * default value should work for you.
 */
#ifndef KERNEL_TICK_LISTS_CNT
#  define KERNEL_TICK_LISTS_CNT    8
#endif


/**
 * API option for `MAKE_ALIG()` macro.
 *
 * There is a terrible mess with `MAKE_ALIG()` macro: original KERNELKernel docs
 * specify that the argument of it should be the size to align, but almost all
 * ports, including "original" one, defined it so that it takes type, not size.
 *
 * But the port by AlexB implemented it differently
 * (i.e. accordingly to the docs)
 *
 * When I was moving from the port by AlexB to another one, do you have any
 * idea how much time it took me to figure out why do I have rare weird bug? :)
 *
 * So, available options:
 *
 *  * `#KERNEL_API_MAKE_ALIG_ARG__TYPE`:
 *             In this case, you should use macro like this:
 *                `KERNEL_MAKE_ALIG(struct my_struct)`
 *             This way is used in the majority of KERNELKernel ports.
 *             (actually, in all ports except the one by AlexB)
 *
 *  * `#KERNEL_API_MAKE_ALIG_ARG__SIZE`:
 *             In this case, you should use macro like this:
 *                `KERNEL_MAKE_ALIG(sizeof(struct my_struct))`
 *             This way is stated in KERNELKernel docs
 *             and used in the port for dsPIC/PIC24/PIC32 by AlexB.
 */
#ifndef KERNEL_API_MAKE_ALIG_ARG
#  define KERNEL_API_MAKE_ALIG_ARG     KERNEL_API_MAKE_ALIG_ARG__SIZE
#endif


/**
 * Whether profiler functionality should be enabled.
 * Enabling this option adds overhead to context switching and increases
 * the size of `#KERNEL_Task` structure by about 20 bytes.
 *
 * @see `#KERNEL_PROFILER_WAIT_TIME`
 * @see `#kernel_task_profiler_timing_get()`
 * @see `struct #KERNEL_TaskTiming`
 */
#ifndef KERNEL_PROFILER
#  define KERNEL_PROFILER            0
#endif

/**
 * Whether profiler should store wait time for each wait reason. Enabling this
 * option bumps the size of `#KERNEL_Task` structure by more than 100 bytes,
 * see `struct #KERNEL_TaskTiming`.
 *
 * Relevant if only `#KERNEL_PROFILER` is non-zero.
 */
#ifndef KERNEL_PROFILER_WAIT_TIME
#  define KERNEL_PROFILER_WAIT_TIME  0
#endif

/**
 * Whether interrupt stack space should be initialized with
 * `#KERNEL_FILL_STACK_VAL` on system start. It is useful to disable this option if
 * you don't want to allocate separate array for interrupt stack, but use
 * initialization stack for it.
 */
#ifndef KERNEL_INIT_INTERRUPT_STACK_SPACE
#  define KERNEL_INIT_INTERRUPT_STACK_SPACE  1
#endif

/**
 * Whether software stack overflow check is enabled.
 *
 * Enabling this option adds small overhead to context switching and system
 * tick processing (`#kernel_tick_int_processing()`), it also reduces the payload
 * of task stacks by just one word (`#KERNEL_UWord`) for each stack.
 *
 * When stack overflow happens, the kernel calls user-provided callback (see
 * `#kernel_callback_stack_overflow_set()`); if this callback is undefined, the
 * kernel calls `#_KERNEL_FATAL_ERROR()`.
 *
 * This option is on by default for all architectures except PIC24/dsPIC,
 * since this architecture has hardware stack pointer limit, unlike the others.
 *
 * \attention
 * It is not an absolute guarantee that the kernel will detect any stack
 * overflow. The kernel tries to detect stack overflow by checking the latest
 * address of stack, which should have special value `#KERNEL_FILL_STACK_VAL`.
 *
 * \attention
 * So stack overflow is detected if only the overflow caused this value to
 * corrupt, which isn't always the case.
 *
 * \attention
 * More, the check is performed only at context switch and timer tick
 * processing, which may be too late.
 *
 * Nevertheless, from my personal experience, it helps to catch stack overflow
 * bugs a lot.
 */
#ifndef KERNEL_STACK_OVERFLOW_CHECK
#  if defined(__KERNEL_ARCH_PIC24_DSPIC__)
/*
 * On PIC24/dsPIC, we have hardware stack pointer limit, so, no need for
 * software check
 */
#     define KERNEL_STACK_OVERFLOW_CHECK   0
#  else
/*
 * On all other architectures, software stack overflow check is ON by default
 */
#     define KERNEL_STACK_OVERFLOW_CHECK   1
#  endif
#endif


/**
 * Whether the kernel should use \ref time_ticks__dynamic_tick scheme instead of
 * \ref time_ticks__static_tick.
 */
#ifndef KERNEL_DYNAMIC_TICK
#  define KERNEL_DYNAMIC_TICK        0
#endif


/**
 * Whether the old KERNELKernel events API compatibility mode is active.
 *
 * \warning Use it if only you're porting your existing KERNELKernel project on
 * KERNEL. Otherwise, usage of this option is strongly discouraged.
 *
 * Actually, events are the most incompatible thing between KERNEL and
 * KERNELKernel (for some details, refer to the section \ref kernelkernel_diff_event)
 *
 * This option is quite useful when you're porting your existing KERNELKernel app
 * to KERNEL. When it is non-zero, old events symbols are available and
 * behave just like they do in KERNELKernel.
 *
 * The full list of what becomes available:
 *
 * - Event group attributes:
 *   - `#KERNEL_EVENT_ATTR_SINGLE`
 *   - `#KERNEL_EVENT_ATTR_MULTI`
 *   - `#KERNEL_EVENT_ATTR_CLR`
 *
 * - Functions:
 *   - `#kernel_event_create()`
 *   - `#kernel_event_delete()`
 *   - `#kernel_event_wait()`
 *   - `#kernel_event_wait_polling()`
 *   - `#kernel_event_iwait()`
 *   - `#kernel_event_set()`
 *   - `#kernel_event_iset()`
 *   - `#kernel_event_clear()`
 *   - `#kernel_event_iclear()`
 */
#ifndef KERNEL_OLD_EVENT_API
#  define KERNEL_OLD_EVENT_API       0
#endif


/**
 * Whether the kernel should use compiler-specific forced inline qualifiers (if
 * possible) instead of "usual" `inline`, which is just a hint for the
 * compiler.
 */
#ifndef KERNEL_FORCED_INLINE
#  define KERNEL_FORCED_INLINE       1
#endif

/**
 * Whether a maximum of reasonable functions should be inlined. Depending of the
 * configuration this may increase the size of the kernel, but it will also
 * improve the performance.
 */
#ifndef KERNEL_MAX_INLINE
#  define KERNEL_MAX_INLINE          0
#endif



/*******************************************************************************
 *    PIC24/dsPIC-specific configuration
 ******************************************************************************/


/**
 * Maximum system interrupt priority. For details on system interrupts on
 * PIC24/dsPIC, refer to the section \ref pic24_interrupts
 * "PIC24/dsPIC interrupts".
 *
 * Should be >= 1 and <= 6. Default: 4.
 */

#ifndef KERNEL_P24_SYS_IPL
#  define KERNEL_P24_SYS_IPL      4
#endif

#endif // _KERNEL_CFG_DEFAULT_H


