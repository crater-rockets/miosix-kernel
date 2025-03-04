/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

#include "interfaces/arch_registers.h"

#ifndef __FPU_PRESENT
#define __FPU_PRESENT 0 //__FPU_PRESENT undefined means no FPU
#endif
#if (__FPU_PRESENT!=0) && (__FPU_USED!=1)
#error "__FPU_USED should be 1"
#endif

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

#if __FPU_PRESENT==1

/// \internal Size in words of vector to store CPU context during context switch
/// ((10+16)*4=104Bytes). Only sp, r4-r11, EXC_RETURN and s16-s31 are saved
/// here, since r0-r3,r12,lr,pc,xPSR, old sp and s0-s15,fpscr are saved by
/// hardware on the process stack on armv7m_fpu CPUs. EXC_RETURN, or the lr,
/// value to use to return from the exception is necessary to know if the
/// thread has used fp regs, as an extension specific to armv7m_fpu CPUs.
const unsigned char CTXSAVE_SIZE=10+16;

/// \internal Size of additional context saved on the stack during context switch.
/// If zero, this architecture does not save anything on stack during context
/// save. Size is in bytes, not words. MUST be divisible by 4. This constant is
/// used to increase the stack size by the size of context save frame.
/// (8+17)*4=100Bytes
///  8 registers=r0-r3,r12,lr,pc,xPSR
/// 17 registers=s0-s15,fpscr
const unsigned int CTXSAVE_ON_STACK=(8+17)*4;

#else //__FPU_PRESENT==1

/// \internal Size in words of vector to store CPU context during context switch
/// (9*4=36Bytes). Only sp and r4-r11 are saved here, since r0-r3,r12,lr,pc,xPSR
/// and old sp are saved by hardware on the process stack on armv7m CPUs.
const unsigned char CTXSAVE_SIZE=9;

/// \internal Size of additional context saved on the stack during context switch.
/// If zero, this architecture does not save anything on stack during context
/// save. Size is in bytes, not words. MUST be divisible by 4. This constant is
/// used to increase the stack size by the size of context save frame.
const unsigned int CTXSAVE_ON_STACK=32;

#endif //__FPU_PRESENT==1

/// \internal Stack alignment required by the CPU
const unsigned int CTXSAVE_STACK_ALIGNMENT=8;

/// \internal Offset in words to retrieve the thread stack pointer in ctxsave
const unsigned int STACK_OFFSET_IN_CTXSAVE=0;

/**
 * \}
 */

} //namespace miosix
