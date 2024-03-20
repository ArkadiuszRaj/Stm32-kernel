/*******************************************************************************
 *
 * KERNEL: real-time kernel initially based on KERNELKernel
 *
 *    KERNELKernel:                  copyright 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright 2013, 2014 Anders Montonen.
 *    KERNEL:                      copyright 2014       Dmitry Frank.
 *
 *    KERNEL was born as a thorough review and re-implementation
 *    of KERNELKernel. New kernel has well-formed code, bugs of ancestor are fixed
 *    as well as new features added, and it is tested carefully with unit-tests.
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
#include "_kernel_eventgrp.h"
#include "_kernel_tasks.h"
#include "_kernel_list.h"


//-- header of current module
#include "kernel_eventgrp.h"

//-- header of other needed modules
#include "kernel_tasks.h"



//TODO: remove in the future
#define  _X96_HACKS  0

/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if KERNEL_CHECK_PARAM
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_generic(
      const struct KERNEL_EventGrp  *eventgrp
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (eventgrp == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_eventgrp_is_valid(eventgrp)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_job_perform(
      const struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpWaitMode       wait_mode,
      KERNEL_UWord                   pattern
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (pattern == 0){
      rc = KERNEL_RC_WPARAM;
   } else {
      wait_mode &= (KERNEL_EVENTGRP_WMODE_OR | KERNEL_EVENTGRP_WMODE_AND);
      if (     wait_mode != KERNEL_EVENTGRP_WMODE_OR
            && wait_mode != KERNEL_EVENTGRP_WMODE_AND)
      {
         rc = KERNEL_RC_WPARAM;
      }
   }

   _KERNEL_UNUSED(eventgrp);

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_create(
      const struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpAttr           attr
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (eventgrp == KERNEL_NULL || _kernel_eventgrp_is_valid(eventgrp)){
      rc = KERNEL_RC_WPARAM;
   }

#if KERNEL_OLD_EVENT_API
   if (!(attr & (KERNEL_EVENTGRP_ATTR_SINGLE | KERNEL_EVENTGRP_ATTR_MULTI))){
      rc = KERNEL_RC_WPARAM;
   } else if (1
         && !(attr & KERNEL_EVENTGRP_ATTR_SINGLE)
         &&  (attr & KERNEL_EVENTGRP_ATTR_CLR)
         )
   {
      rc = KERNEL_RC_WPARAM;
   }
#endif

   _KERNEL_UNUSED(attr);

   return rc;
}

#else
#  define _check_param_generic(eventgrp)                          (KERNEL_RC_OK)
#  define _check_param_job_perform(eventgrp, wait_mode, pattern)  (KERNEL_RC_OK)
#  define _check_param_create(eventgrp, attr)                     (KERNEL_RC_OK)
#endif
// }}}


/**
 * Check if condition is satisfied: check events mask against given pattern.
 *
 * @param eventgrp
 *    Event group to check for condition
 * @param wait_mode
 *    Waiting mode: all specified flags or just any of them; see `enum
 *    #KERNEL_EGrpWaitMode`
 * @param wait_pattern
 *    Pattern to check against
 */
static KERNEL_BOOL _cond_check(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord             wait_pattern
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   KERNEL_BOOL cond = KERNEL_FALSE;

   switch (wait_mode & (KERNEL_EVENTGRP_WMODE_OR | KERNEL_EVENTGRP_WMODE_AND)){
      case KERNEL_EVENTGRP_WMODE_OR:
         //-- any bit set is enough for release condition
         cond = ((eventgrp->pattern & wait_pattern) != 0);
         break;
      case KERNEL_EVENTGRP_WMODE_AND:
         //-- all bits should be set for release condition
         cond = ((eventgrp->pattern & wait_pattern) == wait_pattern);
         break;
#if KERNEL_DEBUG
      default:
         _KERNEL_FATAL_ERROR("invalid wait_mode");
         break;
#endif
   }

   return cond;
}

/**
 * Check if we need to autoclear flag(s), and clear them if we do.
 * This function should be called whenever task successfully ends waiting
 * for flag(s).
 *
 * @param eventgrp
 *    Event group object
 * @param wait_mode
 *    Wait mode, see `enum #KERNEL_EGrpWaitMode`
 * @param pattern
 *    Pattern to clear if we need to.
 */
static void _clear_pattern_if_needed(
      struct KERNEL_EventGrp     *eventgrp,
      enum KERNEL_EGrpWaitMode    wait_mode,
      KERNEL_UWord                pattern
      )
{

#if KERNEL_OLD_EVENT_API
   //-- Old KERNELKernel behavior: there is a flag `KERNEL_EVENTGRP_ATTR_CLR`
   //   belonging to the whole eventgrp object
   if (eventgrp->attr & KERNEL_EVENTGRP_ATTR_CLR){
#if _X96_HACKS
      eventgrp->pattern &= ~pattern;
#else
      eventgrp->pattern = 0;
#endif
   }
#endif

   //-- New KERNEL behavior: there is a flag `KERNEL_EVENTGRP_WMODE_AUTOCLR`
   //   belonging to the particular wait call, so it is specified in
   //   `wait_mode`. See `kernelum KERNEL_EGrpWaitMode`.
   if (wait_mode & KERNEL_EVENTGRP_WMODE_AUTOCLR){
      eventgrp->pattern &= ~pattern;
   }
}


/**
 * Walk through all tasks waiting for some event, wake up tasks whose waiting
 * condition is already satisfied.
 *
 * @param eventgrp
 *    Event group to handle.
 */
static void _scan_event_waitqueue(struct KERNEL_EventGrp *eventgrp)
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   struct KERNEL_Task *task;
   struct KERNEL_Task *tmp_task;

   //-- Walk through all tasks waiting for some event, checking
   //   if each particular condition is satisfied
   _kernel_list_for_each_entry_safe(
         task, struct KERNEL_Task, tmp_task, &(eventgrp->wait_queue), task_queue
         )
   {

      if ( _cond_check(
               eventgrp,
               task->subsys_wait.eventgrp.wait_mode,
               task->subsys_wait.eventgrp.wait_pattern
               )
         )
      {
         //-- Condition is satisfied, so, wake the task up.
         //   We should also remember actual pattern that caused
         //   task to wake up.

         task->subsys_wait.eventgrp.actual_pattern = eventgrp->pattern;
         _kernel_task_wait_complete(task, KERNEL_RC_OK);

         //-- Atomically clear flag(s) if we need to.
         _clear_pattern_if_needed(
               eventgrp,
               task->subsys_wait.eventgrp.wait_mode,
               task->subsys_wait.eventgrp.wait_pattern
               );
      }
   }
}


/**
 * Actual worker function that is eventually called when user calls
 * `kernel_eventgrp_wait()` and friends. It never sleeps; if condition isn't met,
 * then `#KERNEL_RC_TIMEOUT` is returned, and the caller may sleep then (it
 * depends).
 *
 * If condition is met, `#KERNEL_RC_OK` is returned, and the caller will not sleep.
 *
 * For params documentation, refer to the `kernel_eventgrp_wait()`.
 */
static enum KERNEL_RCode _eventgrp_wait(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   enum KERNEL_RCode rc = _check_param_job_perform(
         eventgrp, wait_mode, wait_pattern
         );

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {

#if KERNEL_OLD_EVENT_API
      //-- Old KERNELKernel behavior: if `KERNEL_EVENTGRP_ATTR_SINGLE` flag is set,
      //   only one task can wait for that event group
      if (
            (eventgrp->attr & KERNEL_EVENTGRP_ATTR_SINGLE)
            &&
            !_kernel_list_is_empty(&(eventgrp->wait_queue))
         )
      {
         rc = KERNEL_RC_ILLEGAL_USE;
      } else
#endif

      //-- Check release condition

      if (_cond_check(eventgrp, wait_mode, wait_pattern)){
         //-- condition is met, so, return `#KERNEL_RC_OK`, and we don't need to
         //   wait.

         if (p_flags_pattern != KERNEL_NULL){
            *p_flags_pattern = eventgrp->pattern;

#if _X96_HACKS
            *p_flags_pattern = eventgrp->pattern & wait_pattern;
#endif
         }

         //-- Atomically clear flag(s) if we need to.
         _clear_pattern_if_needed(eventgrp, wait_mode, wait_pattern);
         rc = KERNEL_RC_OK;
      } else {
         //-- The condition isn't met, so, return appropriate code,
         //   and the caller may sleep if it is needed.
         rc = KERNEL_RC_TIMEOUT;
      }

   }
   return rc;
}

/**
 * Actual worker function that is eventually called when user calls
 * `kernel_eventgrp_modify()` or `kernel_eventgrp_imodify()`.
 *
 * Modify current events pattern: set, clear or toggle flags.
 *
 * If flags are cleared, there aren't any side effects: flags are just got
 * cleared. If, however, flags are set or toggled, then all the tasks waiting
 * for some particular event are checked whether the condition is met now. It
 * is done by `_scan_event_waitqueue()`.
 *
 * For params documentation, refer to `kernel_eventgrp_modify()`.
 */
static enum KERNEL_RCode _eventgrp_modify(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpOp       operation,
      KERNEL_UWord             pattern
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   switch (operation){
      case KERNEL_EVENTGRP_OP_CLEAR:
         //-- clear flags: there aren't any side effects: just clear flags.
         eventgrp->pattern &= ~pattern;
         break;

      case KERNEL_EVENTGRP_OP_SET:
         //-- set flags: do that if only given flags aren't already set.
         //   (otherwise, there's no need to spend time walking through
         //   all the waiting tasks)
         if ((eventgrp->pattern & pattern) != pattern){
            //-- flags aren't already set: so, set flags and check all tasks.

            eventgrp->pattern |= pattern;
            _scan_event_waitqueue(eventgrp);
         }
         break;

      case KERNEL_EVENTGRP_OP_TOGGLE:
         //-- toggle flags: after flags are toggled, check all waiting tasks.
         eventgrp->pattern ^= pattern;
         _scan_event_waitqueue(eventgrp);
         break;
   }

   return KERNEL_RC_OK;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_create_wattr(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpAttr     attr,
      KERNEL_UWord             initial_pattern //-- initial value of the pattern
      )
{
   enum KERNEL_RCode rc = _check_param_create(eventgrp, attr);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {

      _kernel_list_reset(&(eventgrp->wait_queue));

      eventgrp->pattern    = initial_pattern;
      eventgrp->id_event   = KERNEL_ID_EVENTGRP;
#if KERNEL_OLD_EVENT_API
      eventgrp->attr       = attr;
#endif

   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_delete(struct KERNEL_EventGrp *eventgrp)
{
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      // remove all waiting tasks from wait list (if any), returning the
      // KERNEL_RC_DELETED code.
      _kernel_wait_queue_notify_deleted(&(eventgrp->wait_queue));

      eventgrp->id_event = KERNEL_ID_NONE; //-- event does not exist now

      KERNEL_INT_RESTORE();

      //-- we might need to switch context if _kernel_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _kernel_context_switch_pend_if_needed();

   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_wait(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern,
      KERNEL_TickCnt           timeout
      )
{
   KERNEL_BOOL waited_for_event = KERNEL_FALSE;
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- call worker function that actually performs needed check
      //   and return result
      rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

      if (rc == KERNEL_RC_TIMEOUT && timeout != 0){
         //-- condition isn't met, and user wants to wait in this case.
         //   So, remember waiting parameters (mode, pattern), and put
         //   current task to wait.

         _kernel_curr_run_task->subsys_wait.eventgrp.wait_mode = wait_mode;
         _kernel_curr_run_task->subsys_wait.eventgrp.wait_pattern = wait_pattern;
         _kernel_task_curr_to_wait_action(
               &(eventgrp->wait_queue),
               KERNEL_WAIT_REASON_EVENT,
               timeout
               );
         waited_for_event = KERNEL_TRUE;
      }

      _KERNEL_BUG_ON(!_kernel_need_context_switch() && waited_for_event);

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();

      if (waited_for_event){
         //-- task was waiting for event, and now it has just woke up.
         //-- get wait result
         rc = _kernel_curr_run_task->task_wait_rc;

         //-- if wait result is KERNEL_RC_OK, and p_flags_pattern is provided,
         //   copy actual_pattern there
         if (rc == KERNEL_RC_OK && p_flags_pattern != KERNEL_NULL ){
            *p_flags_pattern =
               _kernel_curr_run_task->subsys_wait.eventgrp.actual_pattern;

#if _X96_HACKS
            *p_flags_pattern =
               _kernel_curr_run_task->subsys_wait.eventgrp.actual_pattern
               & wait_pattern
               ;
#endif
         }
      }

   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_wait_polling(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern
      )
{
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- call worker function that actually performs needed check
      //   and return result
      rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

      KERNEL_INT_RESTORE();
   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_iwait_polling(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern
      )
{
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();

      //-- call worker function that actually performs needed check
      //   and return result
      rc = _eventgrp_wait(eventgrp, wait_pattern, wait_mode, p_flags_pattern);

      KERNEL_INT_IRESTORE();
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();

   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_modify(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpOp       operation,
      KERNEL_UWord             pattern
      )
{
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- call worker function that actually modifies the events pattern
      rc = _eventgrp_modify(eventgrp, operation, pattern);

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();

   }
   return rc;
}


/*
 * See comments in the header file (kernel_eventgrp.h)
 */
enum KERNEL_RCode kernel_eventgrp_imodify(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpOp       operation,
      KERNEL_UWord             pattern
      )
{
   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();

      //-- call worker function that actually modifies the events pattern
      rc = _eventgrp_modify(eventgrp, operation, pattern);

      KERNEL_INT_IRESTORE();
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }
   return rc;
}




/*******************************************************************************
 *    PROTECTED FUNCTIONS
 ******************************************************************************/

/**
 * See comments in the file _kernel_eventgrp.h
 */
enum KERNEL_RCode _kernel_eventgrp_link_set(
      struct KERNEL_EGrpLink  *eventgrp_link,
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             pattern
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   enum KERNEL_RCode rc = _check_param_generic(eventgrp);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (pattern == (0)){
      rc = KERNEL_RC_WPARAM;
   } else {
      eventgrp_link->eventgrp = eventgrp;
      eventgrp_link->pattern  = pattern;
   }

   return rc;
}


/**
 * See comments in the file _kernel_eventgrp.h
 */
enum KERNEL_RCode _kernel_eventgrp_link_reset(
      struct KERNEL_EGrpLink  *eventgrp_link
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   eventgrp_link->eventgrp = KERNEL_NULL;
   eventgrp_link->pattern  = (0);

   return rc;
}


/**
 * See comments in the file _kernel_eventgrp.h
 */
enum KERNEL_RCode _kernel_eventgrp_link_manage(
      struct KERNEL_EGrpLink  *eventgrp_link,
      KERNEL_BOOL              set
      )
{
   //-- interrupts should be disabled here
   _KERNEL_BUG_ON( !KERNEL_IS_INT_DISABLED() );

   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (eventgrp_link->eventgrp != KERNEL_NULL){
      _eventgrp_modify(
            eventgrp_link->eventgrp,
            (set ? KERNEL_EVENTGRP_OP_SET : KERNEL_EVENTGRP_OP_CLEAR),
            eventgrp_link->pattern
            );
   }

   return rc;
}


