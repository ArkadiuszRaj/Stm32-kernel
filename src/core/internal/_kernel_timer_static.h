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

#ifndef __KERNEL_TIMER_STATIC_H
#define __KERNEL_TIMER_STATIC_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_common.h"
#include "kernel_arch.h"
#include "kernel_list.h"


#if !KERNEL_DYNAMIC_TICK



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

///
/// "generic" list of timers, for details, refer to \ref
/// timers_static_implementation
extern struct KERNEL_ListItem _kernel_timer_list__gen;
///
/// "tick" lists of timers, for details, refer to \ref
/// timers_static_implementation
extern struct KERNEL_ListItem _kernel_timer_list__tick[ KERNEL_TICK_LISTS_CNT ];
///
/// system time that can be returned by `kernel_sys_time_get()`; it is also used
/// by kernel_timer.h subsystem.
extern volatile KERNEL_TickCnt _kernel_sys_time_count;





/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 *    PROTECTED FUNCTION PROTOTYPES
 ******************************************************************************/





/*******************************************************************************
 *    PROTECTED INLINE FUNCTIONS
 ******************************************************************************/

/**
 * Returns current value of system tick counter.
 */
_KERNEL_STATIC_INLINE KERNEL_TickCnt _kernel_timer_sys_time_get(void)
{
   return _kernel_sys_time_count;
}



#ifdef __cplusplus
}  /* extern "C" */
#endif



#endif // !KERNEL_DYNAMIC_TICK

#endif // __KERNEL_TIMER_STATIC_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/



