/***************************************************************************
 *   Copyright (C) 2008-2024 by Terraneo Federico                          *
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

#include "util/util.h"
#include "kernel/logging.h"
#include "interfaces/poweroff.h"
#include "config/miosix_settings.h"

using namespace miosix;

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * This function exists because printf/iprintf cannot be used inside an IRQ.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    char result[]="0x........\r\n";
    formatHex(result+2,x,8);
    IRQerrorLog(result);
}

void default_IRQ_Routine()      __attribute__ ((interrupt("IRQ")));
extern "C" void FIQ_Routine()   __attribute__ ((interrupt("FIQ")));
extern "C" void UNDEF_Routine() __attribute__ ((interrupt("UNDEF")));
extern "C" void DABT_Routine()  __attribute__ ((interrupt("DABT")));
extern "C" void PABT_Routine()  __attribute__ ((interrupt("PABT")));

/**
 * \internal
 * Spurious interrupt handler.
 * The LPC2138 datasheet says that spurious interrups can occur, but until now
 * it never happened. If and when spurious interruts will occur, this code will
 * be modified to deal with them. Until then, this code just reboots the system.
 */
void default_IRQ_Routine()
{
    IRQerrorLog("\r\n***Unexpected IRQ\r\n");
    IRQsystemReboot();
}

/**
 * \internal
 * FIQ is currently not used.
 * Prints an error message, and reboots the system.
 * Stack usage is 24 Bytes (measured with watermarking and stack dump)
 * so a 32 byte stack is used (to leave some guard space).
 * If the user wants to use FIQ, it is important to remember to increase the
 * FIQ's stack size, which is defined in miosix.ld
 */
extern "C" void FIQ_Routine() 
{
    IRQerrorLog("\r\n***Unexpected FIQ\r\n");
    IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles Undefined instruction.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void UNDEF_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt.
    int returnAddress;
    asm volatile("mov	%0, lr" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected UNDEF @ ");
    printUnsignedInt(returnAddress);
    IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles data abort.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void DABT_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 8 during a data abort)
    int returnAddress;
    asm volatile("sub	%0, lr, #8" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected data abort @ ");
    printUnsignedInt(returnAddress);
    IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles prefetch abort.
 * Prints an error message, showing an address near the instruction that caused
 * the exception. This address together with the map file allows finding the
 * function that caused the exception.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 * Stack usage is 47 Bytes (measured with watermarking and stack dump)
 * so a 48 byte stack is used (stak must be word-aligned).
 */
extern "C" void PABT_Routine()
{
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 4 during a data abort)
    int returnAddress;
    asm volatile("sub	%0, lr, #4" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected prefetch abort @ ");
    printUnsignedInt(returnAddress);
    IRQsystemReboot();
}
