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
 * If `#KERNEL_CHECK_BUILD_CFG` option is non-zero, this file needs to be included
 * in the application project.
 *
 * For details, see the aforementioned option `#KERNEL_CHECK_BUILD_CFG`.
 *
 */


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "kernel.h"

//-- std header for memset() that is used inside the macro
//   `_KERNEL_BUILD_CFG_STRUCT_FILL()`
#include <string.h>



#if KERNEL_CHECK_BUILD_CFG


/*******************************************************************************
 *    PRIVATE DATA
 ******************************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

static struct _KERNEL_BuildCfg _build_cfg;

#endif   // DOXYGEN_SHOULD_SKIP_THIS



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * Dummy function that helps user to undefstand that he/she forgot to add file
 * kernel_app_check.c to the project. It is called from kernel_sys.c .
 */
void you_should_add_file___kernel_app_check_c___to_the_project(void)
{
}

/**
 * Return build configuration used for application.
 */
const struct _KERNEL_BuildCfg *kernel_app_build_cfg_get(void)
{
   _KERNEL_BUILD_CFG_STRUCT_FILL(&_build_cfg);
   return &_build_cfg;
}

#endif   // KERNEL_CHECK_BUILD_CFG

