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


#if !KERNEL_DYNAMIC_TICK




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/

//-- see comments in the file _kernel_timer_static.h
struct KERNEL_ListItem _kernel_timer_list__gen;

//-- see comments in the file _kernel_timer_static.h
struct KERNEL_ListItem _kernel_timer_list__tick[ KERNEL_TICK_LISTS_CNT ];

//-- see comments in the file _kernel_timer_static.h
volatile KERNEL_TickCnt _kernel_sys_time_count;




/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define KERNEL_TICK_LISTS_MASK    (KERNEL_TICK_LISTS_CNT - 1)

//-- configuration check
#if ((KERNEL_TICK_LISTS_MASK & KERNEL_TICK_LISTS_CNT) != 0)
#  error KERNEL_TICK_LISTS_CNT must be a power of two
#endif

#if (KERNEL_TICK_LISTS_CNT < 2)
#  error KERNEL_TICK_LISTS_CNT must be >= 2
#endif

//-- The limitation of 256 is actually because struct _KERNEL_BuildCfg has
//   just 8-bit field `tick_lists_cnt_minus_one` for this value.
//   This should never be the problem, because for embedded systems
//   it makes little sense to use even more than 64 lists, as it takes
//   significant amount of RAM.
#if (KERNEL_TICK_LISTS_CNT > 256)
#  error KERNEL_TICK_LISTS_CNT must be <= 256
#endif



/**
 * Return index in the array `#_kernel_timer_list__tick`, based on given timeout.
 *
 * @param timeout    should be < KERNEL_TICK_LISTS_CNT
 */
#define _TICK_LIST_INDEX(timeout)    \
   (((KERNEL_TickCnt)_kernel_sys_time_count + timeout) & KERNEL_TICK_LISTS_MASK)




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/



/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the _kernel_timer.h file.
 */
void _kernel_timers_init(void)
{
   int i;

   //-- reset system time
   _kernel_sys_time_count = 0;

   //-- reset "generic" timers list
   _kernel_list_reset(&_kernel_timer_list__gen);

   //-- reset all "tick" timer lists
   for (i = 0; i < KERNEL_TICK_LISTS_CNT; i++){
      _kernel_list_reset(&_kernel_timer_list__tick[i]);
   }
}

/**
 * See comments in the _kernel_timer.h file.
 */
void _kernel_timers_tick_proceed(KERNEL_UWord KERNEL_INTSAVE_VAR)
{
   //-- first of all, increment system timer
   _kernel_sys_time_count++;

   int tick_list_index = _TICK_LIST_INDEX(0);

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (tick_list_index == 0){
      //-- it happens each KERNEL_TICK_LISTS_CNT-th system tick:
      //   now we should walk through all the timers in the "generic" timer
      //   list, decrement the timeout of each one by KERNEL_TICK_LISTS_CNT,
      //   and if resulting timeout is less than KERNEL_TICK_LISTS_CNT,
      //   then move that timer to the appropriate "tick" timer list.

      //-- handle "generic" timer list {{{
      {
         struct KERNEL_Timer *timer;
         struct KERNEL_Timer *tmp_timer;

         _kernel_list_for_each_entry_safe(
               timer, struct KERNEL_Timer, tmp_timer,
               &_kernel_timer_list__gen, timer_queue
               )
         {

            //-- timeout value should always be >= KERNEL_TICK_LISTS_CNT here.
            //   And it should never be KERNEL_WAIT_INFINITE.
            _KERNEL_BUG_ON(0
                  || timer->timeout_cur == KERNEL_WAIT_INFINITE
                  || timer->timeout_cur < KERNEL_TICK_LISTS_CNT
                  );

            timer->timeout_cur -= KERNEL_TICK_LISTS_CNT;

            if (timer->timeout_cur < KERNEL_TICK_LISTS_CNT){
               //-- it's time to move this timer to the "tick" list
               _kernel_list_remove_entry(&(timer->timer_queue));
               _kernel_list_add_tail(
                     &_kernel_timer_list__tick[_TICK_LIST_INDEX(timer->timeout_cur)],
                     &(timer->timer_queue)
                     );
            }
         }
      }
      //}}}
   }

   //-- it happens every system tick:
   //   we should walk through all the timers in the current "tick" timer list,
   //   and fire them all, unconditionally.

   //-- handle current "tick" timer list {{{
   {
      struct KERNEL_Timer *timer;

      struct KERNEL_ListItem *p_cur_timer_list =
         &_kernel_timer_list__tick[ tick_list_index ];

      //-- now, p_cur_timer_list is a list of timers that we should
      //   fire NOW, unconditionally.

      //-- NOTE that we shouldn't use iterators like
      //   `_kernel_list_for_each_entry_safe()` here, because timers can be
      //   removed from the list while we are iterating through it:
      //   this may happen if user-provided function cancels timer which
      //   is in the same list.
      //
      //   Although timers could be removed from the list, note that
      //   new timer can't be added to it
      //   (because timeout 0 is disallowed, and timer with timeout
      //   KERNEL_TICK_LISTS_CNT is added to the "generic" list),
      //   see implementation details in the kernel_timer.h file
      while (!_kernel_list_is_empty(p_cur_timer_list)){
         timer = _kernel_list_first_entry(
               p_cur_timer_list, struct KERNEL_Timer, timer_queue
               );

         //-- first of all, cancel timer, so that
         //   callback function could start it again if it wants to.
         _kernel_timer_cancel(timer);

         //-- call user callback function
         _kernel_timer_callback_call(timer, KERNEL_INTSAVE_VAR);
      }

      _KERNEL_BUG_ON( !_kernel_list_is_empty(p_cur_timer_list) );
   }
   // }}}
}

/**
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

      //-- if timer is active, cancel it first
      if ((rc = _kernel_timer_cancel(timer)) == KERNEL_RC_OK){

         if (timeout < KERNEL_TICK_LISTS_CNT){
            //-- timer should be added to the one of "tick" lists.
            int tick_list_index = _TICK_LIST_INDEX(timeout);
            timer->timeout_cur = tick_list_index;

            _kernel_list_add_tail(
                  &_kernel_timer_list__tick[ tick_list_index ],
                  &(timer->timer_queue)
                  );
         } else {
            //-- timer should be added to the "generic" list.
            //   We should set timeout_cur adding current "tick" index to it.
            timer->timeout_cur = timeout + _TICK_LIST_INDEX(0);

            _kernel_list_add_tail(&_kernel_timer_list__gen, &(timer->timer_queue));
         }
      }
   }

   return rc;
}

/**
 * See comments in the _kernel_timer.h file.
 */
enum KERNEL_RCode _kernel_timer_cancel(struct KERNEL_Timer *timer)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (_kernel_timer_is_active(timer)){
      //-- reset timeout to zero (but this is actually not necessary)
      timer->timeout_cur = 0;

      //-- remove entry from timer queue
      _kernel_list_remove_entry(&(timer->timer_queue));

      //-- reset the list
      _kernel_list_reset(&(timer->timer_queue));
   }

   return rc;
}

/**
 * See comments in the _kernel_timer.h file.
 */
KERNEL_TickCnt _kernel_timer_time_left(struct KERNEL_Timer *timer)
{
   KERNEL_TickCnt time_left = KERNEL_WAIT_INFINITE;

   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   if (_kernel_timer_is_active(timer)){
      KERNEL_TickCnt tick_list_index = _TICK_LIST_INDEX(0);

      if (timer->timeout_cur > tick_list_index){
         time_left = timer->timeout_cur - tick_list_index;
      } else if (timer->timeout_cur < tick_list_index){
         time_left = timer->timeout_cur + KERNEL_TICK_LISTS_CNT - tick_list_index;
      } else {
#if KERNEL_DEBUG
         //-- timer->timeout_cur should never be equal to tick_list_index here
         //   (if it is, timer is inactive, so, we don't get here)
         _KERNEL_FATAL_ERROR();
#endif
      }
   }

   return time_left;
}


#endif // !KERNEL_DYNAMIC_TICK


