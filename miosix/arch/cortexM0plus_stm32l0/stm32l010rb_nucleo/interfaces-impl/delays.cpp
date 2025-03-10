/***************************************************************************
 *   Copyright (C) 2023 by Federico Terraneo                               *
 *   Copyright (C) 2023 by Daniele Cattaneo                                *
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

#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{    
    for(unsigned int i=0;i<mseconds;i++) delayUs(1000);
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds and
    // it is written in assembler to be independent on compiler optimizations    
    //   Notice that the multiplication factor required is different between
    // CortexM0 and CortexM0+.
    #ifdef SYSCLK_FREQ_32MHz
    unsigned int counter=4;
    #else
    #error "delayUs not implemented"
    #endif
    asm volatile("   mul   %0, %1, %0 \n"
                 "   sub   %0, %0, #1 \n"
                 "   .align 2         \n" // <- important!
                 "1: sub   %0, %0, #1 \n"
                 "   nop              \n"
                 "   cmp   %0, #0     \n" //No subs instruction in cortex m0
                 "   bpl   1b         \n":"+r"(counter):"r"(useconds):"cc");
}

} //namespace miosix

