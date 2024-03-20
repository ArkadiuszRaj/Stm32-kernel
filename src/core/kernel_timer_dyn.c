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

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

//-- common kernelkernel headers
#include "kernel_common.h"
#include "kernel_sys.h"

//-- internal kernelkernel headers
#include "_kernel_timer.h"
#include "_kernel_list.h"


//-- header of current module
#include "kernel_timer.h"

//-- header of other needed modules
#include "kernel_tasks.h"


#if KERNEL_DYNAMIC_TICK




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/

//-- see comments in the file _kernel_timer_dyn.h
KERNEL_CBTickSchedule      *_kernel_cb_tick_schedule = KERNEL_NULL;

//-- see comments in the file _kernel_timer_dyn.h
KERNEL_CBTickCntGet        *_kernel_cb_tick_cnt_get  = KERNEL_NULL;




/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

///
/// List of active non-expired timers. Timers are sorted in ascending order
/// by the timeout value.
static struct KERNEL_ListItem     _timer_list__gen;


/// List of expired timers; after it is initialized, it is used only inside
/// `_kernel_timers_tick_proceed()`
static struct KERNEL_ListItem     _timer_list__fire;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Get pointer to `struct #KERNEL_Task` by pointer to the `task_queue` member
 * of the `struct #KERNEL_Task`.
 */
#define _kernel_get_timer_by_timer_queue(que)                               \
   (que ? container_of(que, struct KERNEL_Timer, timer_queue) : 0)





/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * Get expiration time left. If timer is expired, 0 is returned, no matter
 * how much it is expired.
 */
static KERNEL_TickCnt _time_left_get(
      struct KERNEL_Timer *timer,
      KERNEL_TickCnt cur_sys_tick_cnt
      )
{
   KERNEL_TickCnt time_left;

   //-- Since return value type `KERNEL_TickCnt` is unsigned, we should check if
   //   it is going to be negative. If it is, then return 0.
   KERNEL_TickCnt time_elapsed = cur_sys_tick_cnt - timer->start_tick_cnt;

   if (time_elapsed <= timer->timeout){
      time_left = timer->timeout - time_elapsed;
   } else {
      time_left = 0;
   }

   return time_left;
}

/**
 * Find out when the kernel needs `kernel_tick_int_processing()` to be called next
 * time, and eventually call application callback `_kernel_cb_tick_schedule()` with
 * found value.
 */
static void _next_tick_schedule(KERNEL_TickCnt cur_sys_tick_cnt)
{
   KERNEL_TickCnt next_timeout;

   if (!_kernel_list_is_empty(&_timer_list__gen)){
      //-- need to get first timer from the list and get its timeout
      //   (this list is sorted, so, the first timer is the one with
      //   minimum timeout value)
      struct KERNEL_Timer *timer_next = _kernel_get_timer_by_timer_queue(
            _timer_list__gen.next
            );

      next_timeout = _time_left_get(timer_next, cur_sys_tick_cnt);
   } else {
      //-- no timers are active, so, no ticks needed at all
      next_timeout = KERNEL_WAIT_INFINITE;
   }

   //-- schedule next tick
   _kernel_cb_tick_schedule(next_timeout);
}


/**
 * Cancel the timer: the main thing is that timer is removed from the linked
 * list.
 */
static void _timer_cancel(struct KERNEL_Timer *timer)
{
   //-- reset timeout and start_tick_cnt to zero (but this is actually not
   //   necessary)
   timer->timeout = 0;
   timer->start_tick_cnt = 0;

   //-- remove entry from timer queue
   _kernel_list_remove_entry(&(timer->timer_queue));

   //-- reset the list
   _kernel_list_reset(&(timer->timer_queue));
}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/



/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the _kernel_timer.h file.
 */
void _kernel_timer_dyn_callback_set(
      KERNEL_CBTickSchedule   *cb_tick_schedule,
      KERNEL_CBTickCntGet     *cb_tick_cnt_get
      )
{
   _kernel_cb_tick_schedule = cb_tick_schedule;
   _kernel_cb_tick_cnt_get  = cb_tick_cnt_get;
}


/*
 * See comments in the _kernel_timer.h file.
 */
void _kernel_timers_init(void)
{
   //-- when dynamic tick is used, check that we have callbacks set.
   //   (they should be set by kernel_callback_dyn_tick_set() before calling
   //   kernel_sys_start())
   if (_kernel_cb_tick_schedule == KERNEL_NULL || _kernel_cb_tick_cnt_get == KERNEL_NULL){
      _KERNEL_FATAL_ERROR("");
   }

   //-- reset "generic" timers list
   _kernel_list_reset(&_timer_list__gen);

   //-- reset "current" timers list
   _kernel_list_reset(&_timer_list__fire);
}


/*
 * See comments in the _kernel_timer.h file.
 */
void _kernel_timers_tick_proceed(KERNEL_UWord KERNEL_INTSAVE_VAR)
{
   //-- Since user is allowed to manage timers from timer callbacks, we can't
   //   just iterate through timers list and fire expired timers, because
   //   timers list could change under our feet then.
   //
   //   Instead, we should first move all expired timers to the dedicated
   //   "fire" list, and then, as a second step, fire all the timers from this
   //   list.

   //-- First of all, get current time
   KERNEL_TickCnt cur_sys_tick_cnt = _kernel_timer_sys_time_get();

   //-- Now, walk through timers list from start until we get non-expired timer
   //   (timers list is sorted)
   {
      struct KERNEL_Timer *timer;
      struct KERNEL_Timer *tmp_timer;

      _kernel_list_for_each_entry_safe(
            timer, struct KERNEL_Timer, tmp_timer,
            &_timer_list__gen, timer_queue
            )
      {
         //-- timeout value should never be KERNEL_WAIT_INFINITE.
         _KERNEL_BUG_ON(timer->timeout == KERNEL_WAIT_INFINITE);

         if (_time_left_get(timer, cur_sys_tick_cnt) == 0){
            //-- it's time to fire the timer, so, move it to the "fire" list
            //   `_timer_list__fire`
            _kernel_list_remove_entry(&(timer->timer_queue));
            _kernel_list_add_tail(&_timer_list__fire, &(timer->timer_queue));
         } else {
            //-- We've got non-expired timer, therefore there are no more
            //   expired timers.
            break;
         }
      }
   }

   //-- Now, we have "fire" list containing expired timers. Let's walk
   //   through them, firing each one.
   {
      struct KERNEL_Timer *timer;

      while (!_kernel_list_is_empty(&_timer_list__fire)){
         timer = _kernel_list_first_entry(
               &_timer_list__fire, struct KERNEL_Timer, timer_queue
               );

         //-- first of all, cancel timer *before* calling callback function, so
         //   that function could start it again if it wants to.
         _timer_cancel(timer);

         //-- call user callback function
         _kernel_timer_callback_call(timer, KERNEL_INTSAVE_VAR);
      }
   }

   //-- Find out when `kernel_tick_int_processing()` should be called next time,
   //   and tell that to application
   _next_tick_schedule(cur_sys_tick_cnt);
}

/*
 * See comments in the _kernel_timer.h file.
 */
enum KERNEL_RCode _kernel_timer_start(struct KERNEL_Timer *timer, KERNEL_TickCnt timeout)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (timeout == KERNEL_WAIT_INFINITE || timeout == 0){
      rc = KERNEL_RC_WPARAM;
   } else {
      //-- cancel the timer
      _timer_cancel(timer);

      //-- walk through active timers list and get the position at which
      //   new timer should be placed.

      //-- First of all, get current time
      KERNEL_TickCnt cur_sys_tick_cnt = _kernel_timer_sys_time_get();

      //-- Since timers list is sorted, we need to find the correct place
      //   to put new timer at.
      //
      //   Initially, we set it to the head of the list, and then walk
      //   through timers until we found needed place (or until list is over)
      struct KERNEL_ListItem *list_item = &_timer_list__gen;
      {
         struct KERNEL_Timer *timer;
         struct KERNEL_Timer *tmp_timer;

         _kernel_list_for_each_entry_safe(
               timer, struct KERNEL_Timer, tmp_timer,
               &_timer_list__gen, timer_queue
               )
         {
            //-- timeout value should never be KERNEL_WAIT_INFINITE.
            _KERNEL_BUG_ON(timer->timeout == KERNEL_WAIT_INFINITE);

            if (_time_left_get(timer, cur_sys_tick_cnt) < timeout){
               //-- Probably this is the place for new timer..
               list_item = &timer->timer_queue;
            } else {
               //-- Found timer with larger timeout than that of new timer.
               //   So, list_item now contains the correct place for new timer.
               break;
            }
         }
      }

      //-- put timer object at the right position.
      _kernel_list_add_head(list_item, &(timer->timer_queue));

      //-- initialize timer with given timeout
      timer->timeout = timeout;
      timer->start_tick_cnt = cur_sys_tick_cnt;

      //-- find out when `kernel_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule(cur_sys_tick_cnt);
   }

   return rc;
}

/*
 * See comments in the _kernel_timer.h file.
 */
enum KERNEL_RCode _kernel_timer_cancel(struct KERNEL_Timer *timer)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (_kernel_timer_is_active(timer)){

      //-- cancel the timer
      _timer_cancel(timer);

      //-- find out when `kernel_tick_int_processing()` should be called next time,
      //   and tell that to application
      _next_tick_schedule( _kernel_timer_sys_time_get() );
   }

   return rc;
}

/*
 * See comments in the _kernel_timer.h file.
 */
KERNEL_TickCnt _kernel_timer_time_left(struct KERNEL_Timer *timer)
{
   KERNEL_TickCnt time_left = KERNEL_WAIT_INFINITE;

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (_kernel_timer_is_active(timer)){
      //-- get current time (tick count)
      time_left = _time_left_get(timer, _kernel_timer_sys_time_get());
   }

   return time_left;
}


#endif // !KERNEL_DYNAMIC_TICK



