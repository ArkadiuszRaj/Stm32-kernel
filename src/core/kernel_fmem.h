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
 * Fixed memory blocks pool.
 *
 * A fixed-sized memory blocks pool is used for managing fixed-sized memory
 * blocks dynamically. A pool has a memory area where fixed-sized memory blocks
 * are allocated and the wait queue for acquiring a memory block.  If there are
 * no free memory blocks, a task trying to acquire a memory block will be
 * placed into the wait queue until a free memory block arrives (another task
 * returns it to the memory pool).
 *
 * The operations of getting the block from memory pool and releasing it back
 * take O(1) time independently of number or size of the blocks.
 *
 * For the useful pattern on how to use fixed memory pool together with \ref
 * kernel_dqueue.h "queue", refer to the example: `examples/queue`. Be sure to
 * examine the readme there.
 */

#ifndef _KERNEL_MEM_H
#define _KERNEL_MEM_H

/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel_list.h"
#include "kernel_common.h"



#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Fixed memory blocks pool
 */
struct KERNEL_FMem {
   ///
   /// id for object validity verification.
   /// This field is in the beginning of the structure to make it easier
   /// to detect memory corruption.
   enum KERNEL_ObjId        id_fmp;
   ///
   /// list of tasks waiting for free memory block
   struct KERNEL_ListItem   wait_queue;

   ///
   /// block size (in bytes); note that it should be a multiple of
   /// `sizeof(#KERNEL_UWord})`, use a macro `KERNEL_MAKE_ALIG_SIZE()` for that.
   ///
   /// @see `KERNEL_MAKE_ALIG_SIZE()`
   unsigned int         block_size;
   ///
   /// capacity (total blocks count)
   int                  blocks_cnt;
   ///
   /// free blocks count
   int                  free_blocks_cnt;
   ///
   /// memory pool start address; note that it should be a multiple of
   /// `sizeof(#KERNEL_UWord)`.
   void                *start_addr;
   ///
   /// Pointer to the first free memory block. Each free block contains the
   /// pointer to the next free memory block as the first word, or `NULL` if
   /// this is the last block.
   void                *free_list;
};


/**
 * FMem-specific fields related to waiting task,
 * to be included in struct KERNEL_Task.
 */
struct KERNEL_FMemTaskWait {
   /// if task tries to receive data from memory pool,
   /// and there's no more free blocks in the pool, location to store
   /// pointer is saved in this field
   void *data_elem;
};


/*******************************************************************************
 *    PROTECTED GLOBAL DATA
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/**
 * Convenience macro for the definition of buffer for memory pool. See
 * `kernel_fmem_create()` for usage example.
 *
 * @param name
 *    C variable name of the buffer array (this name should be given
 *    to the `kernel_fmem_create()` function as the `start_addr` argument)
 * @param item_type
 *    Type of item in the memory pool, like `struct MyMemoryItem`.
 * @param size
 *    Number of items in the memory pool.
 */
#define KERNEL_FMEM_BUF_DEF(name, item_type, size)                    \
   KERNEL_UWord name[                                                 \
        (size)                                                    \
      * (KERNEL_MAKE_ALIG_SIZE(sizeof(item_type)) / sizeof(KERNEL_UWord)) \
      ]




/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * Construct fixed memory blocks pool. `id_fmp` field should not contain
 * `#KERNEL_ID_FSMEMORYPOOL`, otherwise, `#KERNEL_RC_WPARAM` is returned.
 *
 * Note that `start_addr` and `block_size` should be a multiple of
 * `sizeof(#KERNEL_UWord)`.
 *
 * For the definition of buffer, convenience macro `KERNEL_FMEM_BUF_DEF()` was
 * invented.
 *
 * Typical definition looks as follows:
 *
 * \code{.c}
 *     //-- number of blocks in the pool
 *     #define MY_MEMORY_BUF_SIZE    8
 *
 *     //-- type for memory block
 *     struct MyMemoryItem {
 *        // ... arbitrary fields ...
 *     };
 *
 *     //-- define buffer for memory pool
 *     KERNEL_FMEM_BUF_DEF(my_fmem_buf, struct MyMemoryItem, MY_MEMORY_BUF_SIZE);
 *
 *     //-- define memory pool structure
 *     struct KERNEL_FMem my_fmem;
 * \endcode
 *
 * And then, construct your `my_fmem` as follows:
 *
 * \code{.c}
 *     enum KERNEL_RCode rc;
 *     rc = kernel_fmem_create( &my_fmem,
 *                          my_fmem_buf,
 *                          KERNEL_MAKE_ALIG_SIZE(sizeof(struct MyMemoryItem)),
 *                          MY_MEMORY_BUF_SIZE );
 *     if (rc != KERNEL_RC_OK){
 *        //-- handle error
 *     }
 * \endcode
 *
 * If given `start_addr` and/or `block_size` aren't aligned properly,
 * `#KERNEL_RC_WPARAM` is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem       pointer to already allocated `struct KERNEL_FMem`.
 * @param start_addr pointer to start of the array; should be aligned properly,
 *                   see example above
 * @param block_size size of memory block; should be a multiple of
 *                   `sizeof(#KERNEL_UWord)`, see example above
 * @param blocks_cnt capacity (total number of blocks in the memory pool)
 *
 * @return
 *    * `#KERNEL_RC_OK` if memory pool was successfully created;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return code
 *      is available: `#KERNEL_RC_WPARAM`.
 *
 * @see KERNEL_MAKE_ALIG_SIZE
 */
enum KERNEL_RCode kernel_fmem_create(
      struct KERNEL_FMem   *fmem,
      void             *start_addr,
      unsigned int      block_size,
      int               blocks_cnt
      );

/**
 * Destruct fixed memory blocks pool.
 *
 * All tasks that wait for free memory block become runnable with
 * `#KERNEL_RC_DELETED` code returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem       pointer to memory pool to be deleted
 *
 * @return
 *    * `#KERNEL_RC_OK` if memory pool is successfully deleted;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 *
 */
enum KERNEL_RCode kernel_fmem_delete(struct KERNEL_FMem *fmem);

/**
 * Get memory block from the pool. Start address of the memory block is returned
 * through the `p_data` argument. The content of memory block is undefined.
 * If there is no free block in the pool, behavior depends on `timeout` value:
 * refer to `#KERNEL_TickCnt`.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_CAN_SLEEP)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem
 *    Pointer to memory pool
 * @param p_data
 *    Address of the `(void *)` to which received block address will be saved
 * @param timeout
 *    Refer to `#KERNEL_TickCnt`
 *
 * @return
 *    * `#KERNEL_RC_OK` if block was successfully returned through `p_data`;
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * Other possible return codes depend on `timeout` value,
 *      refer to `#KERNEL_TickCnt`
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_fmem_get(
      struct KERNEL_FMem *fmem,
      void **p_data,
      KERNEL_TickCnt timeout
      );

/**
 * The same as `kernel_fmem_get()` with zero timeout
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_fmem_get_polling(struct KERNEL_FMem *fmem, void **p_data);

/**
 * The same as `kernel_fmem_get()` with zero timeout, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_fmem_iget_polling(struct KERNEL_FMem *fmem, void **p_data);


/**
 * Release memory block back to the pool. The kernel does not check the
 * validity of the membership of given block in the memory pool.
 * If all the memory blocks in the pool are free already, `#KERNEL_RC_OVERFLOW`
 * is returned.
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem
 *    Pointer to memory pool.
 * @param p_data
 *    Address of the memory block to release.
 *
 * @return
 *    * `#KERNEL_RC_OK` on success
 *    * `#KERNEL_RC_WCONTEXT` if called from wrong context;
 *    * If `#KERNEL_CHECK_PARAM` is non-zero, additional return codes
 *      are available: `#KERNEL_RC_WPARAM` and `#KERNEL_RC_INVALID_OBJ`.
 */
enum KERNEL_RCode kernel_fmem_release(struct KERNEL_FMem *fmem, void *p_data);

/**
 * The same as `kernel_fmem_get()`, but for using in the ISR.
 *
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_CAN_SWITCH_CONTEXT)
 * $(KERNEL_LEGEND_LINK)
 */
enum KERNEL_RCode kernel_fmem_irelease(struct KERNEL_FMem *fmem, void *p_data);

/**
 * Returns number of free blocks in the memory pool
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem
 *    Pointer to memory pool.
 *
 * @return
 *    Number of free blocks in the memory pool, or -1 if wrong params were
 *    given (the check is performed if only `#KERNEL_CHECK_PARAM` is non-zero)
 */
int kernel_fmem_free_blocks_cnt_get(struct KERNEL_FMem *fmem);

/**
 * Returns number of used (non-free) blocks in the memory pool
 *
 * $(KERNEL_CALL_FROM_TASK)
 * $(KERNEL_CALL_FROM_ISR)
 * $(KERNEL_LEGEND_LINK)
 *
 * @param fmem
 *    Pointer to memory pool.
 *
 * @return
 *    Number of used (non-free) blocks in the memory pool, or -1 if wrong
 *    params were given (the check is performed if only `#KERNEL_CHECK_PARAM` is
 *    non-zero)
 */
int kernel_fmem_used_blocks_cnt_get(struct KERNEL_FMem *fmem);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // _KERNEL_MEM_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


