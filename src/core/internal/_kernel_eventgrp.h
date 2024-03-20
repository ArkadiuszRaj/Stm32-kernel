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

#ifndef __KERNEL_EVENTGRP_H
#define __KERNEL_EVENTGRP_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

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
 * Establish link to the event group.
 *
 * \attention Caller must disable interrupts.
 *
 * @param eventgrp_link
 *    eventgrp_link object which should be modified. The structure
 *    `eventgrp_link` is typically contained in some other structure,
 *    for example, #KERNEL_DQueue.
 *
 * @param eventgrp
 *    Event group object to connect
 *
 * @param pattern
 *    Flags pattern that should be maintained by object to which
 *    event group is being connected. Can't be 0.
 */
enum KERNEL_RCode _kernel_eventgrp_link_set(
      struct KERNEL_EGrpLink  *eventgrp_link,
      struct KERNEL_EventGrp  *eventgrp,
      KERNEL_UWord             pattern
      );

/**
 * Reset link to the event group, i.e. make it non connected to any event
 * group. (no matter whether it is already established).
 */
enum KERNEL_RCode _kernel_eventgrp_link_reset(
      struct KERNEL_EGrpLink  *eventgrp_link
      );

/**
 * Set or clear flag(s) in the connected event group, if any. Flag(s) are
 * specified by `pattern` argument given to `#_kernel_eventgrp_link_set()`.
 *
 * If given `eventgrp_link` doesn't contain any connected event group, nothing
 * is performed and no error returned.
 *
 * \attention Caller must disable interrupts.
 *
 * @param eventgrp_link
 *    eventgrp_link object which contains connected event group.
 *    `eventgrp_link` is typically contained in some other structure,
 *    for example, #KERNEL_DQueue.
 *
 * @param set
 *    - if `KERNEL_TRUE`, flag(s) are being set;
 *    - if `KERNEL_FALSE`, flag(s) are being cleared.
 *
 */
enum KERNEL_RCode _kernel_eventgrp_link_manage(
      struct KERNEL_EGrpLink  *eventgrp_link,
      KERNEL_BOOL              set
      );



/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Checks whether given event group object is valid
 * (actually, just checks against `id_event` field, see `enum #KERNEL_ObjId`)
 */
_KERNEL_STATIC_INLINE KERNEL_BOOL _kernel_eventgrp_is_valid(
      const struct KERNEL_EventGrp   *eventgrp
      )
{
   return (eventgrp->id_event == KERNEL_ID_EVENTGRP);
}




#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // __KERNEL_EVENTGRP_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


