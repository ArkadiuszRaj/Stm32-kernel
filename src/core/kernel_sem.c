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
#include "_kernel_tasks.h"
#include "_kernel_list.h"


//-- header of current module
#include "_kernel_sem.h"

//-- header of other needed modules
#include "kernel_tasks.h"




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if KERNEL_CHECK_PARAM
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_generic(
      const struct KERNEL_Sem *sem
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (sem == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_sem_is_valid(sem)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

/**
 * Additional param checking when creating semaphore
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_create(
      const struct KERNEL_Sem *sem,
      int start_count,
      int max_count
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (sem == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (0
         || _kernel_sem_is_valid(sem)
         || max_count <= 0
         || start_count < 0
         || start_count > max_count
         )
   {
      rc = KERNEL_RC_WPARAM;
   }

   return rc;
}

#else
#  define _check_param_generic(sem)                            (KERNEL_RC_OK)
#  define _check_param_create(sem, start_count, max_count)     (KERNEL_RC_OK)
#endif
// }}}


/**
 * Generic function that performs job from task context
 *
 * @param sem        semaphore to perform job on
 * @param p_worker   pointer to actual worker function
 * @param timeout    see `#KERNEL_TickCnt`
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _sem_job_perform(
      struct KERNEL_Sem *sem,
      enum KERNEL_RCode (p_worker)(struct KERNEL_Sem *sem),
      KERNEL_TickCnt timeout
      )
{
   enum KERNEL_RCode rc = _check_param_generic(sem);
   KERNEL_BOOL waited_for_sem = KERNEL_FALSE;

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();      //-- disable interrupts
      rc = p_worker(sem);     //-- call actual worker function

      //-- if we should wait, put current task to wait
      if (rc == KERNEL_RC_TIMEOUT && timeout != 0){
         _kernel_task_curr_to_wait_action(
               &(sem->wait_queue), KERNEL_WAIT_REASON_SEM, timeout
               );

         //-- rc will be set later thanks to waited_for_sem
         waited_for_sem = KERNEL_TRUE;
      }

#if KERNEL_DEBUG
      //-- if we're going to wait, _kernel_need_context_switch() must return KERNEL_TRUE
      if (!_kernel_need_context_switch() && waited_for_sem){
         _KERNEL_FATAL_ERROR("");
      }
#endif

      KERNEL_INT_RESTORE();       //-- restore previous interrupts state
      _kernel_context_switch_pend_if_needed();
      if (waited_for_sem){
         //-- get wait result
         rc = _kernel_curr_run_task->task_wait_rc;
      }

   }
   return rc;
}

/**
 * Generic function that performs job from interrupt context
 *
 * @param sem        semaphore to perform job on
 * @param p_worker   pointer to actual worker function
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _sem_job_iperform(
      struct KERNEL_Sem *sem,
      enum KERNEL_RCode (p_worker)(struct KERNEL_Sem *sem)
      )
{
   enum KERNEL_RCode rc = _check_param_generic(sem);

   //-- perform additional params checking (if enabled by KERNEL_CHECK_PARAM)
   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();     //-- disable interrupts
      rc = p_worker(sem);     //-- call actual worker function
      KERNEL_INT_IRESTORE();      //-- restore previous interrupts state
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }
   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _sem_signal(struct KERNEL_Sem *sem)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- wake up first (if any) task from the semaphore wait queue
   if (  !_kernel_task_first_wait_complete(
            &sem->wait_queue, KERNEL_RC_OK,
            KERNEL_NULL, KERNEL_NULL, KERNEL_NULL
            )
      )
   {
      //-- no tasks are waiting for that semaphore,
      //   so, just increase its count if possible.
      if (sem->count < sem->max_count){
         sem->count++;
      } else {
         rc = KERNEL_RC_OVERFLOW;
      }
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _sem_wait(struct KERNEL_Sem *sem)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- decrement semaphore count if possible.
   //   If not, return KERNEL_RC_TIMEOUT
   //   (it is handled in _sem_job_perform() / _sem_job_iperform())
   if (sem->count > 0){
      sem->count--;
   } else {
      rc = KERNEL_RC_TIMEOUT;
   }

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_create(
      struct KERNEL_Sem *sem,
      int start_count,
      int max_count
      )
{
   //-- perform additional params checking (if enabled by KERNEL_CHECK_PARAM)
   enum KERNEL_RCode rc = _check_param_create(sem, start_count, max_count);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else {

      _kernel_list_reset(&(sem->wait_queue));

      sem->count     = start_count;
      sem->max_count = max_count;
      sem->id_sem    = KERNEL_ID_SEMAPHORE;

   }
   return rc;
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_delete(struct KERNEL_Sem * sem)
{
   //-- perform additional params checking (if enabled by KERNEL_CHECK_PARAM)
   enum KERNEL_RCode rc = _check_param_generic(sem);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- Remove all tasks from wait queue, returning the KERNEL_RC_DELETED code.
      _kernel_wait_queue_notify_deleted(&(sem->wait_queue));

      sem->id_sem = KERNEL_ID_NONE;        //-- Semaphore does not exist now
      KERNEL_INT_RESTORE();

      //-- we might need to switch context if _kernel_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _kernel_context_switch_pend_if_needed();
   }
   return rc;
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_signal(struct KERNEL_Sem *sem)
{
   return _sem_job_perform(sem, _sem_signal, 0);
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_isignal(struct KERNEL_Sem *sem)
{
   return _sem_job_iperform(sem, _sem_signal);
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_wait(struct KERNEL_Sem *sem, KERNEL_TickCnt timeout)
{
   return _sem_job_perform(sem, _sem_wait, timeout);
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_wait_polling(struct KERNEL_Sem *sem)
{
   return _sem_job_perform(sem, _sem_wait, 0);
}

/*
 * See comments in the header file (kernel_sem.h)
 */
enum KERNEL_RCode kernel_sem_iwait_polling(struct KERNEL_Sem *sem)
{
   return _sem_job_iperform(sem, _sem_wait);
}


