/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014 by Terraneo Federico                   *
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#include "interfaces/gpio.h"
#include "interfaces/arch_registers.h"

extern "C" void SystemInit();

namespace miosix {

void IRQmemoryAndClockInit()
{
    //Currently we use the code provided by ST (with our modifications) to
    //handle the clock initialization process.
    SystemInit();

    //Enable all gpios now since we'll need them to initialize the RAM later
    //anyway
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN;
    RCC_SYNC();
    //Port config (H=high, L=low, PU=pullup, PD=pulldown)
    //  |  PORTA  |  PORTB  |  PORTC  |  PORTD  |  PORTE  |  PORTF  |  PORTG  |
    //--+---------+---------+---------+---------+---------+---------+---------+
    // 0| AF11    | AF11    | IN      | AF12    | AF12    | AF12    | AF12    |
    // 1| AF11    | AF11    | AF11    | AF12    | AF12    | AF12    | AF12    |
    // 2| AF11    | IN PD   | AF11    | AF12    | OUT L   | AF12    | AF12    |
    // 3| AF11    | AF0     | AF11    | IN PD   | IN PD   | AF12    | AF12    |
    // 4| OUT H   | AF0     | AF11    | AF12    | IN PD   | AF12    | AF12    |
    // 5| AF5     | AF5     | AF11    | AF12    | IN PD   | AF12    | AF12    |
    // 6| AF5     | IN PD   | OUT L   | IN PD   | IN PD   | OUT L   | IN PD   |
    // 7| AF11    | IN PD   | IN PD   | AF12    | AF12    | IN PU   | IN PD   |
    // 8| AF0     | AF11    | AF12    | AF12    | AF12    | IN PD   | IN PD   |
    // 9| AF7     | IN PD   | AF12    | AF12    | AF12    | IN PD   | IN PD   |
    //10| AF7     | AF11    | AF12    | AF12    | AF12    | IN PD   | IN PD   |
    //11| AF10    | AF11    | AF12    | AF12    | AF12    | IN PD   | IN PD   |
    //12| AF10    | AF11    | AF12    | AF12    | AF12    | AF12    | IN PD   |
    //13| AF0     | AF11    | IN      | IN PD   | AF12    | AF12    | IN PD   |
    //14| AF0     | AF12    | IN PD   | AF12    | AF12    | AF12    | IN PD   |
    //15| AF0     | AF12    | IN PD   | AF12    | AF12    | AF12    | IN PD   |
    
    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOS
    GPIOB->OSPEEDR=0xaaaaaaaa; //Except SRAM GPIOs that run @ 100MHz
    GPIOC->OSPEEDR=0xaaaaaaaa;
    GPIOD->OSPEEDR=0xfbffefaf;
    GPIOE->OSPEEDR=0xffffeaaf;
    GPIOF->OSPEEDR=0xffaaafff;
    GPIOG->OSPEEDR=0xaaaaafff;
    
    GPIOA->MODER=0xaaaaa9aa;
    GPIOB->MODER=0xaaa20a8a;
    GPIOC->MODER=0x02aa1aa8;
    GPIOD->MODER=0xa2aa8a2a;
    GPIOE->MODER=0xaaaa801a;
    GPIOF->MODER=0xaa001aaa;
    GPIOG->MODER=0x00000aaa;
    
    GPIOA->PUPDR=0x64000000;
    GPIOB->PUPDR=0x0008a120;
    GPIOC->PUPDR=0xa0008000;
    GPIOD->PUPDR=0x08002080;
    GPIOE->PUPDR=0x00002a80;
    GPIOF->PUPDR=0x00aa4000;
    GPIOG->PUPDR=0xaaaaa000;
    
    GPIOA->ODR=0x00000010;
    GPIOB->ODR=0x00000000;
    GPIOC->ODR=0x00000000;
    GPIOD->ODR=0x00000000;
    GPIOE->ODR=0x00000000;
    GPIOF->ODR=0x00000000;
    GPIOG->ODR=0x00000000;
    
    GPIOA->AFR[0]=11 | 11<<4 | 11<<8 | 11<<12 |  0<<16 |  5<<20 |  5<<24 | 11<<28;
    GPIOA->AFR[1]= 0 |  7<<4 |  7<<8 | 10<<12 | 10<<16 |  0<<20 |  0<<24 |  0<<28;
    GPIOB->AFR[0]=11 | 11<<4 |  0<<8 |  0<<12 |  0<<16 |  5<<20 |  0<<24 |  0<<28;
    GPIOB->AFR[1]=11 |  0<<4 | 11<<8 | 11<<12 | 11<<16 | 11<<20 | 12<<24 | 12<<28;
    GPIOC->AFR[0]= 0 | 11<<4 | 11<<8 | 11<<12 | 11<<16 | 11<<20 |  0<<24 |  0<<28;
    GPIOC->AFR[1]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 |  0<<20 |  0<<24 |  0<<28;
    GPIOD->AFR[0]=12 | 12<<4 | 12<<8 |  0<<12 | 12<<16 | 12<<20 |  0<<24 | 12<<28;
    GPIOD->AFR[1]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 |  0<<20 | 12<<24 | 12<<28;
    GPIOE->AFR[0]=12 | 12<<4 |  0<<8 |  0<<12 |  0<<16 |  0<<20 |  0<<24 | 12<<28;
    GPIOE->AFR[1]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 | 12<<24 | 12<<28;
    GPIOF->AFR[0]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 |  0<<24 |  0<<28;
    GPIOF->AFR[1]= 0 |  0<<4 |  0<<8 |  0<<12 | 12<<16 | 12<<20 | 12<<24 | 12<<28;
    GPIOG->AFR[0]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 |  0<<24 |  0<<28;
    GPIOG->AFR[1]= 0 |  7<<4 |  7<<8 | 10<<12 | 10<<16 |  0<<20 |  0<<24 |  0<<28;

    //Configure FSMC
    RCC->AHB3ENR=RCC_AHB3ENR_FSMCEN;
    RCC_SYNC();
    volatile uint32_t& BCR1=FSMC_Bank1->BTCR[0];
    volatile uint32_t& BTR1=FSMC_Bank1->BTCR[1];
    BCR1=0x00001011; //16bit width, write enabled, SRAM mode
    BTR1=0x00000200; //DATAST=2
}

} // namespace miosix
