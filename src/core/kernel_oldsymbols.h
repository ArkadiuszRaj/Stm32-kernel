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
 * Compatibility layer for old projects that use old KERNELKernel names;
 * usage of them in new projects is discouraged.
 *
 * If you're porting your existing application written for KERNELKernel, it
 * might be useful though.
 *
 * Included automatially if the option `#KERNEL_OLD_KERNELKERNEL_NAMES` is set.
 *
 */

#ifndef _KERNEL_OLDSYMBOLS_H
#define _KERNEL_OLDSYMBOLS_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_common.h"



#ifndef KERNEL_OLD_KERNELKERNEL_NAMES
#  error KERNEL_OLD_KERNELKERNEL_NAMES is not defined
#endif

#if KERNEL_OLD_KERNELKERNEL_NAMES || DOXYGEN_ACTIVE


#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/// old KERNELKernel name of `KERNEL_ListItem`
typedef struct KERNEL_ListItem    CDLL_QUEUE;

/// old KERNELKernel name of `#KERNEL_Mutex`
typedef struct KERNEL_Mutex       KERNEL_MUTEX;

/// old KERNELKernel name of `#KERNEL_DQueue`
typedef struct KERNEL_DQueue      KERNEL_DQUE;

/// old KERNELKernel name of `#KERNEL_Task`
typedef struct KERNEL_Task        KERNEL_TCB;

/// old KERNELKernel name of `#KERNEL_FMem`
typedef struct KERNEL_FMem        KERNEL_FMP;

/// old KERNELKernel name of `#KERNEL_Sem`
typedef struct KERNEL_Sem         KERNEL_SEM;


#if KERNEL_OLD_EVENT_API
/// old KERNELKernel name of `#KERNEL_EventGrp`
/// available if only `#KERNEL_OLD_EVENT_API` is non-zero
typedef struct KERNEL_EventGrp    KERNEL_EVENT;
#endif




/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * compatibility with old struct names
 */
/// old KERNELKernel struct name of `KERNEL_ListItem`
#define  _CDLL_QUEUE    KERNEL_ListItem

/// old KERNELKernel struct name of `#KERNEL_Mutex`
#define  _KERNEL_MUTEX      KERNEL_Mutex

/// old KERNELKernel struct name of `#KERNEL_DQueue`
#define  _KERNEL_DQUE       KERNEL_DQueue

/// old KERNELKernel struct name of `#KERNEL_Task`
#define  _KERNEL_TCB        KERNEL_Task

/// old KERNELKernel struct name of `#KERNEL_FMem`
#define  _KERNEL_FMP        KERNEL_FMem

/// old KERNELKernel struct name of `#KERNEL_Sem`
#define  _KERNEL_SEM        KERNEL_Sem

#if KERNEL_OLD_EVENT_API || defined(DOXYGEN_ACTIVE)
/// old KERNELKernel struct name of `#KERNEL_EventGrp`,
/// available if only `#KERNEL_OLD_EVENT_API` is non-zero
#  define  _KERNEL_EVENT      KERNEL_EventGrp
#endif

/// old KERNELKernel name of `#KERNEL_MAKE_ALIG` macro
///
/// \attention it is recommended to use `#KERNEL_MAKE_ALIG_SIZE` macro instead
/// of this one, in order to avoid confusion caused by various
/// KERNELKernel ports: refer to the section \ref kernelkernel_diff_make_alig for details.
#define  MAKE_ALIG                     KERNEL_MAKE_ALIG


/// old KERNELKernel name of `#KERNEL_TASK_STATE_RUNNABLE`
#define  TSK_STATE_RUNNABLE            KERNEL_TASK_STATE_RUNNABLE

/// old KERNELKernel name of `#KERNEL_TASK_STATE_WAIT`
#define  TSK_STATE_WAIT                KERNEL_TASK_STATE_WAIT

/// old KERNELKernel name of `#KERNEL_TASK_STATE_SUSPEND`
#define  TSK_STATE_SUSPEND             KERNEL_TASK_STATE_SUSPEND

/// old KERNELKernel name of `#KERNEL_TASK_STATE_WAITSUSP`
#define  TSK_STATE_WAITSUSP            KERNEL_TASK_STATE_WAITSUSP

/// old KERNELKernel name of `#KERNEL_TASK_STATE_DORMANT`
#define  TSK_STATE_DORMANT             KERNEL_TASK_STATE_DORMANT

/// old KERNELKernel name of `#KERNEL_TASK_CREATE_OPT_START`
#define  KERNEL_TASK_START_ON_CREATION     KERNEL_TASK_CREATE_OPT_START

/// old KERNELKernel name of `#KERNEL_TASK_EXIT_OPT_DELETE`
#define  KERNEL_EXIT_AND_DELETE_TASK       KERNEL_TASK_EXIT_OPT_DELETE



/// old KERNELKernel name of `#KERNEL_EVENTGRP_WMODE_AND`
#define  KERNEL_EVENT_WCOND_AND            KERNEL_EVENTGRP_WMODE_AND

/// old KERNELKernel name of `#KERNEL_EVENTGRP_WMODE_OR`
#define  KERNEL_EVENT_WCOND_OR             KERNEL_EVENTGRP_WMODE_OR


/// old KERNELKernel name of `#KERNEL_WAIT_REASON_NONE`
#define  TSK_WAIT_REASON_NONE          KERNEL_WAIT_REASON_NONE

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_SLEEP`
#define  TSK_WAIT_REASON_SLEEP         KERNEL_WAIT_REASON_SLEEP

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_SEM`
#define  TSK_WAIT_REASON_SEM           KERNEL_WAIT_REASON_SEM

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_EVENT`
#define  TSK_WAIT_REASON_EVENT         KERNEL_WAIT_REASON_EVENT

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_DQUE_WSEND`
#define  TSK_WAIT_REASON_DQUE_WSEND    KERNEL_WAIT_REASON_DQUE_WSEND

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_DQUE_WRECEIVE`
#define  TSK_WAIT_REASON_DQUE_WRECEIVE KERNEL_WAIT_REASON_DQUE_WRECEIVE

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_MUTEX_C`
#define  TSK_WAIT_REASON_MUTEX_C       KERNEL_WAIT_REASON_MUTEX_C

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_MUTEX_I`
#define  TSK_WAIT_REASON_MUTEX_I       KERNEL_WAIT_REASON_MUTEX_I

/// old KERNELKernel name of `#KERNEL_WAIT_REASON_WFIXMEM`
#define  TSK_WAIT_REASON_WFIXMEM       KERNEL_WAIT_REASON_WFIXMEM



/// old KERNELKernel name of `#KERNEL_RC_OK`
#define  TERR_NO_ERR                   KERNEL_RC_OK

/// old KERNELKernel name of `#KERNEL_RC_OVERFLOW`
#define  TERR_OVERFLOW                 KERNEL_RC_OVERFLOW

/// old KERNELKernel name of `#KERNEL_RC_WCONTEXT`
#define  TERR_WCONTEXT                 KERNEL_RC_WCONTEXT

/// old KERNELKernel name of `#KERNEL_RC_WSTATE`
#define  TERR_WSTATE                   KERNEL_RC_WSTATE

/// old KERNELKernel name of `#KERNEL_RC_TIMEOUT`
#define  TERR_TIMEOUT                  KERNEL_RC_TIMEOUT

/// old KERNELKernel name of `#KERNEL_RC_WPARAM`
#define  TERR_WRONG_PARAM              KERNEL_RC_WPARAM

/// old KERNELKernel name of `#KERNEL_RC_ILLEGAL_USE`
#define  TERR_ILUSE                    KERNEL_RC_ILLEGAL_USE

/// old KERNELKernel name of `#KERNEL_RC_INVALID_OBJ`
#define  TERR_NOEXS                    KERNEL_RC_INVALID_OBJ

/// old KERNELKernel name of `#KERNEL_RC_DELETED`
#define  TERR_DLT                      KERNEL_RC_DELETED

/// old KERNELKernel name of `#KERNEL_RC_FORCED`
#define  TERR_FORCED                   KERNEL_RC_FORCED

/// old KERNELKernel name of `#KERNEL_RC_INTERNAL`
#define  TERR_INTERNAL                 KERNEL_RC_INTERNAL



/// old KERNELKernel name of `#KERNEL_MUTEX_PROT_CEILING`
#define  KERNEL_MUTEX_ATTR_CEILING         KERNEL_MUTEX_PROT_CEILING

/// old KERNELKernel name of `#KERNEL_MUTEX_PROT_INHERIT`
#define  KERNEL_MUTEX_ATTR_INHERIT         KERNEL_MUTEX_PROT_INHERIT




/// old KERNELKernel name of `#kernel_sem_acquire_polling`
#define  kernel_sem_polling                kernel_sem_acquire_polling

/// old KERNELKernel name of `#kernel_sem_iacquire_polling`
#define  kernel_sem_ipolling               kernel_sem_iacquire_polling


/// old name of `#kernel_sem_wait`
#define  kernel_sem_acquire                kernel_sem_wait

/// old name of `#kernel_sem_wait_polling`
#define  kernel_sem_acquire_polling        kernel_sem_wait_polling

/// old name of `#kernel_sem_iwait_polling`
#define  kernel_sem_iacquire_polling       kernel_sem_iwait_polling



/// old KERNELKernel name of `#kernel_fmem_iget_polling`
#define  kernel_fmem_get_ipolling          kernel_fmem_iget_polling


/// old KERNELKernel name of `#kernel_queue_ireceive_polling`
#define  kernel_queue_ireceive             kernel_queue_ireceive_polling


/// old KERNELKernel name of `#kernel_sys_start`
#define  kernel_start_system               kernel_sys_start

/// old KERNELKernel name of `#kernel_sys_tslice_set`
#define  kernel_sys_tslice_ticks           kernel_sys_tslice_set



/// old KERNELKernel name of `#KERNEL_ARCH_STK_ATTR_BEFORE`
#define  align_attr_start              KERNEL_ARCH_STK_ATTR_BEFORE

/// old KERNELKernel name of `#KERNEL_ARCH_STK_ATTR_AFTER`
#define  align_attr_end                KERNEL_ARCH_STK_ATTR_AFTER


/// old KERNELKernel name of `#kernel_arch_int_dis`
#define  kernel_cpu_int_disable            kernel_arch_int_dis

/// old KERNELKernel name of `#kernel_arch_int_en`
#define  kernel_cpu_int_enable             kernel_arch_int_en


/// old KERNELKernel name of `#kernel_arch_sr_save_int_dis`
#define  kernel_cpu_save_sr                kernel_arch_sr_save_int_dis

/// old KERNELKernel name of `#kernel_arch_sr_restore`
#define  kernel_cpu_restore_sr             kernel_arch_sr_restore


/// old KERNELKernel name of `#KERNEL_INT_DIS_SAVE`
#define  kernel_disable_interrupt          KERNEL_INT_DIS_SAVE

/// old KERNELKernel name of `#KERNEL_INT_RESTORE`
#define  kernel_enable_interrupt           KERNEL_INT_RESTORE


/// old KERNELKernel name of `#KERNEL_INT_IDIS_SAVE`
#define  kernel_idisable_interrupt         KERNEL_INT_IDIS_SAVE

/// old KERNELKernel name of `#KERNEL_INT_IRESTORE`
#define  kernel_ienable_interrupt          KERNEL_INT_IRESTORE


/// old KERNELKernel name of `#KERNEL_IS_INT_DISABLED`
#define  kernel_chk_irq_disabled           KERNEL_IS_INT_DISABLED

/// old KERNELKernel name of `#KERNEL_PRIORITIES_CNT`
#define  KERNEL_NUM_PRIORITY               KERNEL_PRIORITIES_CNT

/// old KERNELKernel name of `#KERNEL_INT_WIDTH`
#define  _KERNEL_BITS_IN_INT                KERNEL_INT_WIDTH

/// old KERNELKernel name for `sizeof(#KERNEL_UWord)`
#define  KERNEL_ALIG                       sizeof(KERNEL_UWord)

/// old KERNELKernel name for `#KERNEL_NO_TIME_SLICE`
#define  NO_TIME_SLICE                 KERNEL_NO_TIME_SLICE

/// old KERNELKernel name for `#KERNEL_MAX_TIME_SLICE`
#define  MAX_TIME_SLICE                KERNEL_MAX_TIME_SLICE




/// old name for `#KERNEL_STACK_ARR_DEF`
#define  KERNEL_TASK_STACK_DEF             KERNEL_STACK_ARR_DEF

/// old name for `#KERNEL_TickCnt`
#define  KERNEL_Timeout                    KERNEL_TickCnt



//-- event stuff {{{

#if KERNEL_OLD_EVENT_API || DOXYGEN_ACTIVE

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#KERNEL_EVENTGRP_ATTR_SINGLE`,
#define  KERNEL_EVENT_ATTR_SINGLE          KERNEL_EVENTGRP_ATTR_SINGLE

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#KERNEL_EVENTGRP_ATTR_MULTI`,
#define  KERNEL_EVENT_ATTR_MULTI           KERNEL_EVENTGRP_ATTR_MULTI

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#KERNEL_EVENTGRP_ATTR_CLR`,
#define  KERNEL_EVENT_ATTR_CLR             KERNEL_EVENTGRP_ATTR_CLR

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#kernel_eventgrp_create_wattr()`,
#define  kernel_event_create(ev, attr, pattern)  \
         kernel_eventgrp_create_wattr((ev), (enum KERNEL_EGrpAttr)(attr), (pattern))

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#kernel_eventgrp_delete()`,
#define  kernel_event_delete               kernel_eventgrp_delete

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#kernel_eventgrp_wait()`,
#define  kernel_event_wait                 kernel_eventgrp_wait

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#kernel_eventgrp_wait_polling()`,
#define  kernel_event_wait_polling         kernel_eventgrp_wait_polling

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old name for `#kernel_eventgrp_iwait_polling()`,
#define  kernel_event_iwait                kernel_eventgrp_iwait_polling

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old KERNELKernel-compatible way of calling `#kernel_eventgrp_modify (event,
/// #KERNEL_EVENTGRP_OP_SET, pattern)`
#define  kernel_event_set(ev, pattern)     kernel_eventgrp_modify ((ev), KERNEL_EVENTGRP_OP_SET, (pattern))

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old KERNELKernel-compatible way of calling `#kernel_eventgrp_imodify (event,
/// #KERNEL_EVENTGRP_OP_SET, pattern)`
#define  kernel_event_iset(ev, pattern)    kernel_eventgrp_imodify((ev), KERNEL_EVENTGRP_OP_SET, (pattern))

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old KERNELKernel-compatible way of calling `#kernel_eventgrp_modify (event,
/// #KERNEL_EVENTGRP_OP_CLEAR, (~pattern))`
///
/// \attention Unlike `#kernel_eventgrp_modify()`, the pattern should be inverted!
#define  kernel_event_clear(ev, pattern)   kernel_eventgrp_modify ((ev), KERNEL_EVENTGRP_OP_CLEAR, (~(pattern)))

/// \attention Deprecated. Available if only `#KERNEL_OLD_EVENT_API` option is
/// non-zero.
///
/// Old KERNELKernel-compatible way of calling `#kernel_eventgrp_imodify (event,
/// #KERNEL_EVENTGRP_OP_CLEAR, (~pattern))`
///
/// \attention Unlike `#kernel_eventgrp_modify()`, the pattern should be inverted!
#define  kernel_event_iclear(ev, pattern)  kernel_eventgrp_imodify((ev), KERNEL_EVENTGRP_OP_CLEAR, (~(pattern)))

#else
//-- no compatibility with event API
#endif

//}}}


//-- Internal implementation details {{{

//-- Exclude it from doxygen-generated docs
#if !DOXYGEN_SHOULD_SKIP_THIS

#define kernel_ready_list         _kernel_tasks_ready_list
#define kernel_create_queue       _kernel_tasks_created_list
#define kernel_created_tasks_cnt  _kernel_tasks_created_cnt

#define kernel_tslice_ticks       _kernel_tslice_ticks

#define kernel_sys_state          _kernel_sys_state

#define kernel_next_task_to_run   _kernel_next_task_to_run
#define kernel_curr_run_task      _kernel_curr_run_task

#define kernel_ready_to_run_bmp   _kernel_ready_to_run_bmp

#define kernel_idle_task          _kernel_idle_task

#endif

//}}}

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // KERNEL_OLD_KERNELKERNEL_NAMES

#endif // _KERNEL_OLDSYMBOLS_H


/*******************************************************************************
 *    end of file
 ******************************************************************************/


