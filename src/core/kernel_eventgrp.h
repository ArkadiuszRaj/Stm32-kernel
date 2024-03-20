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
 * Event group.
 *
 * An event group has an internal variable (of type `#KERNEL_UWord`), which is
 * interpreted as a bit pattern where each bit represents an event. An event
 * group also has a wait queue for the tasks waiting on these events. A task
 * may set specified bits when an event occurs and may clear specified bits
 * when necessary.
 *
 * The tasks waiting for an event(s) are placed in the event group's wait
 * queue. An event group is a very suitable synchronization object for cases
 * where (for some reasons) one task has to wait for many tasks, or vice versa,
 * many tasks have to wait for one task.
 *
 * \section eventgrp_connect Connecting an event group to other system objects
 *
 * Sometimes task needs to wait for different system events, the most common
 * examples are:
 *
 * - wait for a message from the queue(s) plus wait for some
 *   application-dependent event (such as a flag to finish the task, or
 *   whatever);
 * - wait for messages from multiple queues.
 *
 * If the kernel doesn't offer a mechanism for that, programmer usually have to
 * use polling services on these queues and sleep for a few system ticks.
 * Obviously, this approach has serious drawbacks: we have a lot of useless
 * context switches, and response for the message gets much slower. Actually,
 * we lost the main goal of the preemptive kernel when we use polling services
 * like that.
 *
 * KERNEL offers a solution: an event group can be connected to other
 * kernel objects, and these objects will maintain certain flags inside that
 * event group automatically.
 *
 * So, in case of multiple queues, we can act as follows (assume we have two
 * queues: Q1 and Q2) :
 *
 * - create event group EG;
 * - connect EG with flag 1 to Q1;
 * - connect EG with flag 2 to Q2;
 * - when task needs to receive a message from either Q1 or Q2, it just waits
 *   for the any of flags 1 or 2 in the EG, this is done in the single call
 *   to `kernel_eventgrp_wait()`.
 * - when that event happened, task checks which flag is set, and receive
 *   message from the appropriate queue.
 *
 * Please note that task waiting for the event should **not** clear the flag
 * manually: this flag is maintained completely by the queue. If the queue is
 * non-empty, the flag is set. If the queue becomes empty, the flag is cleared.
 *
 * For the information on system services related to queue, refer to the \ref
 * kernel_dqueue.h "queue reference".
 *
 * There is an example project available that demonstrates event group
 * connection technique: `examples/queue_eventgrp_conn`. Be sure to examine the
 * readme there.
 *
 */

#ifndef _KERNEL_EVENTGRP_H
#define _KERNEL_EVENTGRP_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_list.h"
#include "kernel_common.h"
#include "kernel_sys.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Events waiting mode that should be given to `#kernel_eventgrp_wait()` and
 * friends.
 */
enum KERNEL_EGrpWaitMode {
   ///
   /// Task waits for **any** of the event bits from the `wait_pattern`
   /// to be set in the event group.
   /// This flag is mutually exclusive with `#KERNEL_EVENTGRP_WMODE_AND`.
   KERNEL_EVENTGRP_WMODE_OR       = (1 << 0),
   ///
   /// Task waits for **all** of the event bits from the `wait_pattern`
   /// to be set in the event group.
   /// This flag is mutually exclusive with `#KERNEL_EVENTGRP_WMODE_OR`.
   KERNEL_EVENTGRP_WMODE_AND      = (1 << 1),
   ///
   /// When a task <b>successfully</b> ends waiting for event bit(s), these
   /// bits get cleared atomically and automatically. Other bits stay
   /// unchanged.
   KERNEL_EVENTGRP_WMODE_AUTOCLR  = (1 << 2),
};

/**
 * Modify operation: set, clear or toggle. To be used in `kernel_eventgrp_modify()`
 * / `kernel_eventgrp_imodify()` functions.
 */
enum KERNEL_EGrpOp {
   ///
   /// Set flags that are set in given `pattern` argument. Note that this
   /// operation can lead to the context switch, since other high-priority
   /// task(s) might wait for the event.
   KERNEL_EVENTGRP_OP_SET,
   ///
   /// Clear flags that are set in the given `pattern` argument.
   /// This operation can **not** lead to the context switch,
   /// since tasks can't wait for events to be cleared.
   KERNEL_EVENTGRP_OP_CLEAR,
   ///
   /// Toggle flags that are set in the given `pattern` argument. Note that this
   /// operation can lead to the context switch, since other high-priority
   /// task(s) might wait for the event that was just set (if any).
   KERNEL_EVENTGRP_OP_TOGGLE,
};


/**
 * Attributes that could be given to the event group object.
 *
 * Makes sense if only `#KERNEL_OLD_EVENT_API` option is non-zero; otherwise,
 * there's just one dummy attribute available: `#KERNEL_EVENTGRP_ATTR_NONE`.
 */
enum KERNEL_EGrpAttr {
#if KERNEL_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   /// \attention deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
   /// non-zero.
   ///
   /// Indicates that only one task could wait for events in this event group.
   /// This flag is mutually exclusive with `#KERNEL_EVENTGRP_ATTR_MULTI` flag.
   KERNEL_EVENTGRP_ATTR_SINGLE    = (1 << 0),
   ///
   /// \attention deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
   /// non-zero.
   ///
   /// Indicates that multiple tasks could wait for events in this event group.
   /// This flag is mutually exclusive with `#KERNEL_EVENTGRP_ATTR_SINGLE` flag.
   KERNEL_EVENTGRP_ATTR_MULTI     = (1 << 1),
   ///
   /// \attention strongly deprecated. Available if only `#KERNEL_OLD_EVENT_API`
   /// option is non-zero. Use `#KERNEL_EVENTGRP_WMODE_AUTOCLR` instead.
   ///
   /// Can be specified only in conjunction with `#KERNEL_EVENTGRP_ATTR_SINGLE`
   /// flag. Indicates that <b>ALL</b> flags in this event group should be
   /// cleared when task successfully waits for any event in it.
   ///
   /// This actually makes little sense to clear ALL events, but this is what
   /// compatibility mode is for (see `#KERNEL_OLD_EVENT_API`)
   KERNEL_EVENTGRP_ATTR_CLR       = (1 << 2),
#endif

#if !KERNEL_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   ///
   /// Dummy attribute that does not change anything. It is needed only for
   /// the assistance of the events compatibility mode (see
   /// `#KERNEL_OLD_EVENT_API`)
   KERNEL_EVENTGRP_ATTR_NONE      = (0),
#endif
};


/**
 * Event group
 */
struct KERNEL_EventGrp {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum KERNEL_ObjId        id_event;
   ///
   /// task wait queue
   struct KERNEL_ListItem   wait_queue;
   ///
   /// current flags pattern
   KERNEL_UWord             pattern;

#if KERNEL_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
   ///
   /// Attributes that are given to that events group,
   /// available if only `#KERNEL_OLD_EVENT_API` option is non-zero.
   enum KERNEL_EGrpAttr     attr;
#endif

};

/**
 * EventGrp-specific fields related to waiting task,
 * to be included in struct KERNEL_Task.
 */
struct KERNEL_EGrpTaskWait {
   ///
   /// event wait pattern
   KERNEL_UWord wait_pattern;
   ///
   /// event wait mode: `AND` or `OR`
   enum KERNEL_EGrpWaitMode wait_mode;
   ///
   /// pattern that caused task to finish waiting
   KERNEL_UWord actual_pattern;
};

/**
 * A link to event group: used when event group can be connected to
 * some kernel object, such as queue.
 */
struct KERNEL_EGrpLink {
   ///
   /// event group whose event(s) should be managed by other kernel object
   struct KERNEL_EventGrp *eventgrp;
   ///
   /// event pattern to manage
   KERNEL_UWord pattern;
};


/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * The same as `#kernel_eventgrp_create()`, but takes additional argument: `attr`.
 * It makes sense if only `#KERNEL_OLD_EVENT_API` option is non-zero.
 *
 * @param eventgrp
 *    Pointer to already allocated struct KERNEL_EventGrp
 * @param attr
 *    Attributes for that particular event group object, see `struct
 *    #KERNEL_EGrpAttr`
 * @param initial_pattern
 *    Initial events pattern.
 */
enum KERNEL_RCode kernel_eventgrp_create_wattr(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpAttr     attr,
      KERNEL_UWord             initial_pattern
      );

/**
 * Construct event group. `id_event` field should not contain `#KERNEL_ID_EVENTGRP`,
 * otherwise, `#KERNEL_RC_WPARAM` is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to already allocated struct KERNEL_EventGrp
 * @param initial_pattern
 *    Initial events pattern.
 *
 * @return
 *    * `#KERNEL_RC_OK` if event group was successfully created;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#KERNEL_RC_WPARAM`.
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode kernel_eventgrp_create(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             initial_pattern
      )
{
   return kernel_eventgrp_create_wattr(
         eventgrp,
#if KERNEL_OLD_EVENT_API
         (KERNEL_EVENTGRP_ATTR_MULTI),
#else
         (KERNEL_EVENTGRP_ATTR_NONE),
#endif
         initial_pattern
         );
}

/**
 * Destruct event group.
 *
 * All tasks that wait for the event(s) become runnable with `#KERNEL_RC_DELETED`
 * code returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param eventgrp   Pointer to event groupt to be deleted.
 *
 * @return
 *    * `#KERNEL_RC_OK` if event group was successfully deleted;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_eventgrp_delete(struct KERNEL_EventGrp *eventgrp);

/**
 * Wait for specified event(s) in the event group. If the specified event
 * is already active, function returns `#KERNEL_RC_OK` immediately. Otherwise,
 * behavior depends on `timeout` value: refer to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to event group to wait events from
 * @param wait_pattern
 *    Events bit pattern for which task should wait
 * @param wait_mode
 *    Specifies whether task should wait for **all** the event bits from
 *    `wait_pattern` to be set, or for just **any** of them
 *    (see enum `#KERNEL_EGrpWaitMode`)
 * @param p_flags_pattern
 *    Pointer to the `KERNEL_UWord` variable in which actual event pattern
 *    that caused task to stop waiting will be stored.
 *    May be `KERNEL_NULL`.
 * @param timeout
 *    refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK` if specified event is active (so the task can check
 *      variable pointed to by `p_flags_pattern` if it wasn't `KERNEL_NULL`).
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_eventgrp_wait(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern,
      KERNEL_TickCnt           timeout
      );

/**
 * The same as `kernel_eventgrp_wait()` with zero timeout.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_eventgrp_wait_polling(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern
      );

/**
 * The same as `kernel_eventgrp_wait()` with zero timeout, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_eventgrp_iwait_polling(
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             wait_pattern,
      enum KERNEL_EGrpWaitMode wait_mode,
      KERNEL_UWord            *p_flags_pattern
      );

/**
 * Modify current events bit pattern in the event group. Behavior depends
 * on the given `operation`: refer to `enum #KERNEL_EGrpOp`
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param eventgrp
 *    Pointer to event group to modify events in
 * @param operation
 *    Actual operation to perform: set, clear or toggle.
 *    Refer to `enum #KERNEL_EGrpOp`
 * @param pattern
 *    Events pattern to be applied (depending on `operation` value)
 *
 * @return
 *    * `#KERNEL_RC_OK` on success;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_eventgrp_modify(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpOp       operation,
      KERNEL_UWord             pattern
      );

/**
 * The same as `kernel_eventgrp_modify()`, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_eventgrp_imodify(
      struct KERNEL_EventGrp  *eventgrp,
      enum KERNEL_EGrpOp       operation,
      KERNEL_UWord             pattern
      );


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_EVENTGRP_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


