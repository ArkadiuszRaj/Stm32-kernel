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
#include "kernel_fmem.h"
#include "_kernel_fmem.h"

//-- header of other needed modules
#include "kernel_tasks.h"



/*******************************************************************************
 *    EXTERNAL DATA
 ******************************************************************************/




/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

//-- Additional param checking {{{
#if KERNEL_CHECK_PARAM
_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_fmem_create(
      const struct KERNEL_FMem *fmem
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (fmem == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (_kernel_fmem_is_valid(fmem)){
      rc = KERNEL_RC_WPARAM;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_fmem_delete(
      const struct KERNEL_FMem *fmem
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (fmem == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_fmem_is_valid(fmem)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_job_perform(
      const struct KERNEL_FMem *fmem,
      void *p_data
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (fmem == KERNEL_NULL || p_data == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_fmem_is_valid(fmem)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}

_KERNEL_STATIC_INLINE enum KERNEL_RCode _check_param_generic(
      const struct KERNEL_FMem *fmem
      )
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   if (fmem == KERNEL_NULL){
      rc = KERNEL_RC_WPARAM;
   } else if (!_kernel_fmem_is_valid(fmem)){
      rc = KERNEL_RC_INVALID_OBJ;
   }

   return rc;
}
#else
#  define _check_param_fmem_create(fmem)               (KERNEL_RC_OK)
#  define _check_param_fmem_delete(fmem)               (KERNEL_RC_OK)
#  define _check_param_job_perform(fmem, p_data)       (KERNEL_RC_OK)
#  define _check_param_generic(fmem)                   (KERNEL_RC_OK)
#endif
// }}}

/**
 * Callback function that is given to `_kernel_task_first_wait_complete()`
 * when task finishes waiting for free block in the memory pool.
 *
 * See `#_KERNEL_CBBeforeTaskWaitComplete` for details on function signature.
 */
static void _cb_before_task_wait_complete(
      struct KERNEL_Task   *task,
      void             *user_data_1,
      void             *user_data_2
      )
{
   task->subsys_wait.fmem.data_elem = user_data_1;
   _KERNEL_UNUSED(user_data_2);
}

/**
 * Try to allocate memory block from the pool.
 *
 * If there is free memory block, it is allocated and the address of it is
 * stored at the provided location (`p_data`). Otherwise (no free blocks),
 * `#KERNEL_RC_TIMEOUT` is returned, and this case can be handled by the caller.
 *
 * @param fmem
 *    Memory pool from which block should be taken
 * @param p_data
 *    Pointer to where the result should be stored (if `#KERNEL_RC_TIMEOUT` is
 *    returned, this location isn't altered)
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _fmem_get(struct KERNEL_FMem *fmem, void **p_data)
{
   enum KERNEL_RCode rc;
   void *ptr = KERNEL_NULL;

   if (fmem->free_blocks_cnt > 0){
      //-- Get first block from the pool
      ptr = fmem->free_list;

      //-- Alter pointer to the first block: make it point to the next free
      //   block.
      //
      //   Each free memory block contains the pointer to the next free memory
      //   block as the first word, or `NULL` if it is the last free block.
      //
      //   So, just read that word and save to `free_list`.
      fmem->free_list = *(void **)fmem->free_list;

      //-- And just decrement free blocks count.
      fmem->free_blocks_cnt--;

      //-- Store pointer to newly allocated memory block to the user-provided
      //   location.
      *p_data = ptr;

      rc = KERNEL_RC_OK;
   } else {
      //-- There are no free memory blocks.
      rc = KERNEL_RC_TIMEOUT;
   }

   return rc;
}

/**
 * Return memory block to the pool.
 *
 * First of all, check if there is some task that waits for free block
 * in the pool. If there is, the block is given to it, task is woken up, and
 * the memory pool isn't altered at all.
 *
 * If there aren't waiting tasks, then put memory block to the pool.
 *
 * @param fmem
 *    Memory pool
 * @param p_data
 *    Pointer to the memory block to release.
 *
 * @return
 *    - `#KERNEL_RC_OK`, if operation was successful
 *    - `#KERNEL_RC_OVERFLOW`, if memory pool already has all its blocks free.
 *      This may never happen in normal program execution; if that happens,
 *      it's a programmer's mistake.
 */
_KERNEL_STATIC_INLINE enum KERNEL_RCode _fmem_release(struct KERNEL_FMem *fmem, void *p_data)
{
   enum KERNEL_RCode rc = KERNEL_RC_OK;

   //-- Check if there are tasks waiting for memory block. If there is,
   //   give the block to the first task from the queue.
   if (  !_kernel_task_first_wait_complete(
            &fmem->wait_queue, KERNEL_RC_OK,
            _cb_before_task_wait_complete, p_data, KERNEL_NULL
            )
      )
   {
      //-- no task is waiting for free memory block, so,
      //   insert in to the memory pool

      if (fmem->free_blocks_cnt < fmem->blocks_cnt){
         //-- Insert block into free block list.
         //   See comments inside `_fmem_get()` for more detailed explanation
         //   of how the kernel keeps track of free blocks.
         *(void **)p_data = fmem->free_list;
         fmem->free_list = p_data;
         fmem->free_blocks_cnt++;
      } else {
#if KERNEL_DEBUG
         if (fmem->free_blocks_cnt > fmem->blocks_cnt){
            _KERNEL_FATAL_ERROR(
                  "free_blocks_cnt should never be more than blocks_cnt"
                  );
         }
#endif
         //-- the memory pool already has all the blocks free
         rc = KERNEL_RC_OVERFLOW;
      }
   }

   return rc;
}





/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_create(
      struct KERNEL_FMem   *fmem,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      )
{
   enum KERNEL_RCode rc;

   rc = _check_param_fmem_create(fmem);
   if (rc != KERNEL_RC_OK){
      goto out;
   }

   //-- basic check: start_addr should not be KERNEL_NULL,
   //   and blocks_cnt should be at least 2
   if (start_addr == KERNEL_NULL || blocks_cnt < 2){
      rc = KERNEL_RC_WPARAM;
      goto out;
   }

   //-- check that start_addr is aligned properly
   {
      KERNEL_UIntPtr start_addr_aligned
         = KERNEL_MAKE_ALIG_SIZE((KERNEL_UIntPtr)start_addr);

      if (start_addr_aligned != (KERNEL_UIntPtr)start_addr){
         rc = KERNEL_RC_WPARAM;
         goto out;
      }
   }

   //-- check that block_size is aligned properly
   {
      unsigned int block_size_aligned = KERNEL_MAKE_ALIG_SIZE(block_size);
      if (block_size_aligned != block_size){
         rc = KERNEL_RC_WPARAM;
         goto out;
      }
   }

   //-- checks are done; proceed to actual creation

   fmem->start_addr = start_addr;
   fmem->block_size = block_size;
   fmem->blocks_cnt = blocks_cnt;

   //-- reset wait_queue
   _kernel_list_reset(&(fmem->wait_queue));

   //-- init block pointers
   {
      void **p_tmp;
      unsigned char *p_block;
      int i;

      p_tmp    = (void **)fmem->start_addr;
      p_block  = (unsigned char *)fmem->start_addr + fmem->block_size;
      for (i = 0; i < (fmem->blocks_cnt - 1); i++){
         *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
         p_tmp   = (void **)p_block;
         p_block += fmem->block_size;
      }
      *p_tmp = KERNEL_NULL;          //-- Last memory block first cell contents -  KERNEL_NULL

      fmem->free_list       = fmem->start_addr;
      fmem->free_blocks_cnt = fmem->blocks_cnt;
   }

   //-- set id
   fmem->id_fmp = KERNEL_ID_FSMEMORYPOOL;

out:
   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_delete(struct KERNEL_FMem *fmem)
{
   enum KERNEL_RCode rc = _check_param_fmem_delete(fmem);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      //-- remove all tasks (if any) from fmem's wait queue
      _kernel_wait_queue_notify_deleted(&(fmem->wait_queue));

      fmem->id_fmp = KERNEL_ID_NONE;   //-- Fixed-size memory pool does not exist now

      KERNEL_INT_RESTORE();

      //-- we might need to switch context if _kernel_wait_queue_notify_deleted()
      //   has woken up some high-priority task
      _kernel_context_switch_pend_if_needed();

   }
   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_get(
      struct KERNEL_FMem *fmem,
      void **p_data,
      KERNEL_TickCnt timeout
      )
{
   KERNEL_BOOL waited_for_data = KERNEL_FALSE;
   enum KERNEL_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      rc = _fmem_get(fmem, p_data);

      if (rc == KERNEL_RC_TIMEOUT && timeout > 0){
         _kernel_task_curr_to_wait_action(
               &(fmem->wait_queue),
               KERNEL_WAIT_REASON_WFIXMEM,
               timeout
               );
         waited_for_data = KERNEL_TRUE;
      }

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();
      if (waited_for_data){

         //-- get wait result
         rc = _kernel_curr_run_task->task_wait_rc;

         //-- if wait result is KERNEL_RC_OK, copy memory block pointer to the
         //   user's location
         if (rc == KERNEL_RC_OK){
            *p_data = _kernel_curr_run_task->subsys_wait.fmem.data_elem;
         }

      }

   }
   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_get_polling(struct KERNEL_FMem *fmem,void **p_data)
{
   enum KERNEL_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();
      rc = _fmem_get(fmem, p_data);
      KERNEL_INT_RESTORE();
   }

   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_iget_polling(struct KERNEL_FMem *fmem, void **p_data)
{
   enum KERNEL_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();

      rc = _fmem_get(fmem, p_data);

      KERNEL_INT_IRESTORE();
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }
   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_release(struct KERNEL_FMem *fmem, void *p_data)
{
   enum KERNEL_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_task_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA;

      KERNEL_INT_DIS_SAVE();

      rc = _fmem_release(fmem, p_data);

      KERNEL_INT_RESTORE();
      _kernel_context_switch_pend_if_needed();
   }
   return rc;
}


/*
 * See comments in the header file (kernel_dqueue.h)
 */
enum KERNEL_RCode kernel_fmem_irelease(struct KERNEL_FMem *fmem, void *p_data)
{
   enum KERNEL_RCode rc = _check_param_job_perform(fmem, p_data);

   if (rc != KERNEL_RC_OK){
      //-- just return rc as it is
   } else if (!kernel_is_isr_context()){
      rc = KERNEL_RC_WCONTEXT;
   } else {
      KERNEL_INTSAVE_DATA_INT;

      KERNEL_INT_IDIS_SAVE();

      rc = _fmem_release(fmem, p_data);

      KERNEL_INT_IRESTORE();
      _KERNEL_CONTEXT_SWITCH_IPEND_IF_NEEDED();
   }

   return rc;
}

/*
 * See comments in the header file (kernel_dqueue.h)
 */
int kernel_fmem_free_blocks_cnt_get(struct KERNEL_FMem *fmem)
{
   int ret = -1;

   enum KERNEL_RCode rc = _check_param_generic(fmem);
   if (rc == KERNEL_RC_OK){
      //-- It's not needed to disable interrupts here, since `free_blocks_cnt`
      //   is read by just one assembler instruction
      ret = fmem->free_blocks_cnt;
   }

   return ret;
}

/*
 * See comments in the header file (kernel_dqueue.h)
 */
int kernel_fmem_used_blocks_cnt_get(struct KERNEL_FMem *fmem)
{
   int ret = -1;

   enum KERNEL_RCode rc = _check_param_generic(fmem);
   if (rc == KERNEL_RC_OK){
      //-- It's not needed to disable interrupts here, since `free_blocks_cnt`
      //   is read by just one assembler instruction, and `blocks_cnt` never
      //   changes.
      ret = fmem->blocks_cnt - fmem->free_blocks_cnt;
   }

   return ret;
}

