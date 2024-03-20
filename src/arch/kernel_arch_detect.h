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
 * Define macros for platform identification.
 */

#ifndef _KERNEL_ARCH_DETECT_H
#define _KERNEL_ARCH_DETECT_H



/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/


#undef __KERNEL_ARCH_PIC24_DSPIC__
#undef __KERNEL_ARCH_PIC32MX__
#undef __KERNEL_ARCH_CORTEX_M__
#undef __KERNEL_ARCH_CORTEX_M0__
#undef __KERNEL_ARCH_CORTEX_M3__
#undef __KERNEL_ARCH_CORTEX_M4__
#undef __KERNEL_ARCH_CORTEX_M4_FP__

#undef __KERNEL_ARCHFEAT_CORTEX_M_FPU__
#undef __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#undef __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#undef __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__

#undef __KERNEL_COMPILER_ARMCC__
#undef __KERNEL_COMPILER_IAR__
#undef __KERNEL_COMPILER_GCC__
#undef __KERNEL_COMPILER_CLANG__



#if defined(__PIC24F__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__PIC24E__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__PIC24H__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__dsPIC30F__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__dsPIC33E__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__dsPIC33F__)

#  define __KERNEL_ARCH_PIC24_DSPIC__

#elif defined (__PIC32MX__)

#  define __KERNEL_ARCH_PIC32MX__

#elif defined (__ARMCC_VERSION)

#  define __KERNEL_ARCH_CORTEX_M__
#  define __KERNEL_COMPILER_ARMCC__

#  if defined(__TARGET_CPU_CORTEX_M0)
#     define __KERNEL_ARCH_CORTEX_M0__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__

/*
 * For Cortex-M0+, ARMCC defines pretty nasty macro __TARGET_CPU_CORTEX_M0_,
 * see some details here:
 * http://stackoverflow.com/questions/25973956/predefined-cpu-target-macro-for-cortex-m0
 */
#  elif defined(__TARGET_CPU_CORTEX_M0_)
#     define __KERNEL_ARCH_CORTEX_M0__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#  elif defined(__TARGET_CPU_CORTEX_M3)
#     define __KERNEL_ARCH_CORTEX_M3__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#  elif defined(__TARGET_CPU_CORTEX_M4)
#     define __KERNEL_ARCH_CORTEX_M4__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#  elif defined(__TARGET_CPU_CORTEX_M4_FP)
#     define __KERNEL_ARCH_CORTEX_M4_FP__
#     define __KERNEL_ARCHFEAT_CORTEX_M_FPU__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#  else
#     error unknown architecture for ARMCC compiler
#  endif

#elif defined (__IAR_SYSTEMS_ICC__) || defined (__IAR_SYSTEMS_ASM__)

#  if defined(__ICCARM__) || defined(__IASMARM__)
#     define __KERNEL_COMPILER_IAR__

#     if !defined(__CORE__)
#        error __CORE__ is not defined
#     endif

#     if defined(__ARM_PROFILE_M__)
#        define __KERNEL_ARCH_CORTEX_M__
#     else
#        error IAR: the only supported core family is Cortex-M
#     endif

#     if (__CORE__ == __ARM6M__)
#        define __KERNEL_ARCH_CORTEX_M0__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     elif (__CORE__ == __ARM7M__)
#        define __KERNEL_ARCH_CORTEX_M3__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     elif (__CORE__ == __ARM7EM__)
#        if !defined(__ARMVFP__)
#           define __KERNEL_ARCH_CORTEX_M4__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        else
#           define __KERNEL_ARCH_CORTEX_M4_FP__
#           define __KERNEL_ARCHFEAT_CORTEX_M_FPU__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        endif
#     else
#        error __CORE__ is unsupported
#     endif

#  else
#     error unknown compiler of IAR
#  endif

#elif defined(__GNUC__)

#  if defined(__clang__)
#     define __KERNEL_COMPILER_CLANG__
#  else
#     define __KERNEL_COMPILER_GCC__
#  endif

#  if defined(__ARM_ARCH)

#     define __KERNEL_ARCH_CORTEX_M__

#     if defined(__ARM_ARCH_6M__)
#        define __KERNEL_ARCH_CORTEX_M0__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     elif defined(__ARM_ARCH_7M__)
#        define __KERNEL_ARCH_CORTEX_M3__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#        define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     elif defined(__ARM_ARCH_7EM__)
#        if defined(__SOFTFP__)
#           define __KERNEL_ARCH_CORTEX_M4__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        else
#           define __KERNEL_ARCH_CORTEX_M4_FP__
#           define __KERNEL_ARCHFEAT_CORTEX_M_FPU__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __KERNEL_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        endif
#     else
#        error unknown ARM architecture for GCC compiler
#     endif
#  else
#     error unknown architecture for GCC compiler
#  endif
#else
#  error unknown platform
#endif










#endif  /* _KERNEL_ARCH_DETECT_H */

