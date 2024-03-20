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




/*******************************************************************************
 *    PROTECTED DATA
 ******************************************************************************/



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if KERNEL_CHECK_PARAM
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_generic(
      const struct KERNEL_Timer *timer
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (timer == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_timer_is_valid(timer)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_create(
      const struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (timer == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (_kernel_timer_is_valid(timer)){
      rc = KERNEL_RC_WPARAM;
   }

   _KERNEL_UNUSED(func);

   return rc;
}

#else
#  define _check_param_generic(timer)           (KERNEL_RC_OK)
#  define _check_param_create(timer, func)      (KERNEL_RC_OK)
#endif
// }}}



/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_create(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum KERNEL_RCode rc = _check_param_create(timer, func);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {
      rc = _kernel_timer_create(timer, func, p_user_data);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_delete(struct KERNEL_Timer *timer)
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      //-- if timer is active, cancel it first
      rc = _kernel_timer_cancel(timer);

      //-- now, delete timer
      timer->id_timer = KERNEL_ID_NONE;
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_start(struct KERNEL_Timer *timer, KERNEL_TickCnt timeout)
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      rc = _kernel_timer_start(timer, timeout);
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_cancel(struct KERNEL_Timer *timer)
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      rc = _kernel_timer_cancel(timer);
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_set_func(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      )
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      rc = _kernel_timer_set_func(timer, func, p_user_data);
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_is_active(struct KERNEL_Timer *timer, KERNEL_BOOL *p_is_active)
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      *p_is_active = _kernel_timer_is_active(timer);
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}

/*
 * See comments in the header file (kernel_timer.h)
 */
enum KERNEL_RCode kernel_timer_time_left(
      struct KERNEL_Timer *timer,
      KERNEL_TickCnt *p_time_left
      )
{
   KERNEL_UWord sr_saved;
   enum KERNEL_RCode rc = _check_param_generic(timer);

   if (rc == KERNEL_RC_OK){
      sr_saved = kernel_arch_sr_save_int_dis();
      *p_time_left = _kernel_timer_time_left(timer);
      kernel_arch_sr_restore(sr_saved);
   }

   return rc;
}





/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/


/**
 * See comments in the _kernel_timer.h file.
 */
enum KERNEL_RCode _kernel_timer_create(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum KERNEL_RCode rc = _kernel_timer_set_func(timer, func, p_user_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {

      _kernel_list_reset(&(timer->timer_queue));

#if KERNEL_DYNAMIC_TICK
      timer->timeout = 0;
      timer->start_tick_cnt = 0;
#else
      timer->timeout_cur   = 0;
#endif
      timer->id_timer      = KERNEL_ID_TIMER;

   }
   return rc;
}

/**
 * See comments in the _kernel_timer.h file.
 */
enum KERNEL_RCode _kernel_timer_set_func(
      struct KERNEL_Timer  *timer,
      KERNEL_TimerFunc     *func,
      void             *p_user_data
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (func == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else {
      timer->func          = func;
      timer->p_user_data   = p_user_data;
   }

   return rc;
}

/**
 * See comments in the _kernel_timer.h file.
 */
KERNEL_BOOL _kernel_timer_is_active(struct KERNEL_Timer *timer)
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   return (!_kernel_list_is_empty(&(timer->timer_queue)));
}


