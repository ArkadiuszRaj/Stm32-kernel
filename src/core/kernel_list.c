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

#include "kernel_common.h"
#include "_kernel_list.h"



#if (!KERNEL_MAX_INLINE || defined(_KERNEL_MAX_INLINE_INCLUDE_SOURCE))

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC void _kernel_list_reset(struct KERNEL_ListItem *list)
{
   list->prev = list->next = list;
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC KERNEL_BOOL _kernel_list_is_empty(struct KERNEL_ListItem *list)
{
   return (list->next == list && list->prev == list);
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC void _kernel_list_add_head(struct KERNEL_ListItem *list, struct KERNEL_ListItem *entry)
{
  //--  Insert an entry at the head of the queue.

   entry->next = list->next;
   entry->prev = list;
   entry->next->prev = entry;
   list->next = entry;
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC void _kernel_list_add_tail(struct KERNEL_ListItem *list, struct KERNEL_ListItem *entry)
{
  //-- Insert an entry at the tail of the queue.

   entry->next = list;
   entry->prev = list->prev;
   entry->prev->next = entry;
   list->prev = entry;
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC struct KERNEL_ListItem *_kernel_list_remove_head(struct KERNEL_ListItem *list)
{
   //-- Remove and return an entry at the head of the queue.

   struct KERNEL_ListItem *entry = KERNEL_NULL;

   if (list != KERNEL_NULL && list->next != list){
      entry             = list->next;
      entry->next->prev = list;
      list->next        = entry->next;
   } else {
      //-- KERNEL_NULL will be returned
   }

   return entry;
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC struct KERNEL_ListItem *_kernel_list_remove_tail(struct KERNEL_ListItem *list)
{
   //-- Remove and return an entry at the tail of the queue.

   struct KERNEL_ListItem *entry;

   if (list->prev == list){
      return (struct KERNEL_ListItem *) 0;
   }

   entry = list->prev;
   entry->prev->next = list;
   list->prev = entry->prev;

   return entry;
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC void _kernel_list_remove_entry(struct KERNEL_ListItem *entry)
{
   //--  Remove an entry from the queue.
   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;

   //-- NOTE: we shouldn't reset the entry itself, because the pointers in
   //         the removed item are often needed to get the next item of the
   //         list from which it was just removed.
   //
   //         Typical usage: iterate over the list, check some condition and
   //         probably remove the item. After removal, we are still able
   //         to get the next item.
}

/*
 * See comments in the header file kernel_list.h
 */
_KERNEL_MAX_INLINED_FUNC KERNEL_BOOL _kernel_list_contains_entry(
      struct KERNEL_ListItem *list,
      struct KERNEL_ListItem *entry
      )
{
   KERNEL_BOOL ret = KERNEL_FALSE;
   struct KERNEL_ListItem *item;

   _kernel_list_for_each(item, list){
      if (item == entry){
         ret = KERNEL_TRUE;
         break;
      }
   }

   return ret;
}

#endif // KERNEL_MAX_INLINE


