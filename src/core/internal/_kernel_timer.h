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

#ifndef __KERNEL_TIMER_H
#define __KERNEL_TIMER_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- as long as that header file (_kernel_timer.h) is a generic one for timers,
//   we should include headers of each particular timers implementation as well
//   (static timers and dynamic timers, it depends on KERNEL_DYNAMIC_TICK option
//   which one is actually used)
#include "_kernel_timer_static.h"
#include "_kernel_timer_dyn.h"


#include "_kernel_sys.h"





#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/






/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Should be called once at system startup (from `#kernel_sys_start()`).
 * It merely resets all timer lists.
 */
void _kernel_timers_init(void);

/**
 * Should be called from $(KERNEL_SYS_TIMER_LINK) interrupt. It performs all
 * necessary timers housekeeping: moving them between lists, firing them, etc.
 *
 * See \ref timers_static_implementation for details.
 */
void _kernel_timers_tick_proceed(KERNEL_UWord KERNEL_INTSAVE_VAR);

/**
 * Actual worker function that is called by `#kernel_timer_start()`.
 * Interrupts should be disabled when calling it.
 */
enum KERNEL_RCode _kernel_timer_start(struct KERNEL_Timer *timer, KERNEL_TickCnt timeout);

/**
 * Actual worker function that is called by `#kernel_timer_cancel()`.
 * Interrupts should be disabled when calling it.
 */
enum KERNEL_RCode _kernel_timer_cancel(struct KERNEL_Timer *timer);

/**
 * Actual worker function that is called by `#kernel_timer_create()`.
 */
enum KERNEL_RCode _kernel_timer_create(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Actual worker function that is called by `#kernel_timer_set_func()`.
 */
enum KERNEL_RCode _kernel_timer_set_func(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      );

/**
 * Actual worker function that is called by `#kernel_timer_is_active()`.
 * Interrupts should be disabled when calling it.
 */
KERNEL_BOOL _kernel_timer_is_active(struct KERNEL_Timer *timer);

/**
 * Actual worker function that is called by `#kernel_timer_time_left()`.
 * Interrupts should be disabled when calling it.
 */
KERNEL_TickCnt _kernel_timer_time_left(struct KERNEL_Timer *timer);





/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given timer object is valid
 * (actually, just checks against `id_timer` field, see `enum #KERNEL_ObjId`)
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_timer_is_valid(
      const struct KERNEL_Timer   *timer
      )
{
   return (timer->id_timer == KERNEL_ID_TIMER);
}

/**
 * Called by `_kernel_timers_tick_proceed()`, which is implemented differently
 * depending on `KERNEL_DYNAMIC_TICK` option.
 *
 * Enables interrupts, calls callback function, disables interrupts back.
 *
 * @param timer
 *    Timer to operate on
 * @param KERNEL_INTSAVE_VAR
 *    Status of interrupts, used by `KERNEL_INT_IDIS_SAVE()` and friends.
 */
_KERNEL_STATIC_INLINE void _kernel_timer_callback_call(
      struct KERNEL_Timer  *timer,
      KERNEL_UWord          KERNEL_INTSAVE_VAR
      )
{
   //-- we're going to enable interrupt before calling callback, so,
   //   remember user data before enabling them, since the structure
   //   might be changed by interrupt
   void *p_user_data = timer->p_user_data;

   //-- before calling callback function, enable interrupts, so that
   //   they aren't disabled for too long
   KERNEL_INT_IRESTORE();

   //-- call user callback function
   timer->func(timer, p_user_data);

   //-- after callback is done, disable interrupts back
   //   (saved value won't be used by anyone though)
   KERNEL_INT_IDIS_SAVE();
}


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __KERNEL_TIMER_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


