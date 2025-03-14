/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
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

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <cstdlib>
#include <inttypes.h>
#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "interfaces_private/bsp_private.h"
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "interfaces/delays.h"
#include "interfaces/poweroff.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "kernel/logging.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"
#include "drivers/serial.h"
#include "drivers/sd_stm32f2_f4_f7.h"
#include "board_settings.h"
#include "hwmapping.h"

namespace miosix {

//
// Initialization
//

/**
 * The example code from ST checks for the busy flag after each command.
 * Interestingly I couldn't find any mention of this in the datsheet.
 */
static void sdramCommandWait()
{
    for(int i=0;i<0xffff;i++)
        if((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY)==0) return;
}

void configureSdram()
{
    //Enable all gpios, passing clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;
    RCC_SYNC();
    
    //First, configure SDRAM GPIOs
    GPIOB->AFR[0]=0x0cc00000;
    GPIOC->AFR[0]=0x0000000c;
    GPIOD->AFR[0]=0x000000cc;
    GPIOD->AFR[1]=0xcc000ccc;
    GPIOE->AFR[0]=0xc00000cc;
    GPIOE->AFR[1]=0xcccccccc;
    GPIOF->AFR[0]=0x00cccccc;
    GPIOF->AFR[1]=0xccccc000;
    GPIOG->AFR[0]=0x00cc00cc;
    GPIOG->AFR[1]=0xc000000c;

    GPIOB->MODER=0x00002800;
    GPIOC->MODER=0x00000002;
    GPIOD->MODER=0xa02a000a;
    GPIOE->MODER=0xaaaa800a;
    GPIOF->MODER=0xaa800aaa;
    GPIOG->MODER=0x80020a0a;
    
    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOs...
    GPIOB->OSPEEDR=0xaaaaaaaa | 0x00003c00; //...but 100MHz for the SDRAM pins
    GPIOC->OSPEEDR=0xaaaaaaaa | 0x00000003;
    GPIOD->OSPEEDR=0xaaaaaaaa | 0xf03f000f;
    GPIOE->OSPEEDR=0xaaaaaaaa | 0xffffc00f;
    GPIOF->OSPEEDR=0xaaaaaaaa | 0xffc00fff;
    GPIOG->OSPEEDR=0xaaaaaaaa | 0xc0030f0f;
    GPIOH->OSPEEDR=0xaaaaaaaa;
    
    //Since we'we un-configured PB3/PB4 from the default at boot TDO,NTRST,
    //finish the job and remove the default pull-up
    GPIOB->PUPDR=0;
    
    //Second, actually start the SDRAM controller
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();
    
    //SDRAM is a IS42S16400J -7 speed grade, connected to bank 2 (0xd0000000)
    //Some bits in SDCR[1] are don't care, and the have to be set in SDCR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDCR[0]=FMC_SDCR1_SDCLK_1// SDRAM runs @ half CPU frequency
                       | FMC_SDCR1_RBURST // Enable read burst
                       | 0;               //  0 delay between reads after CAS
    FMC_Bank5_6->SDCR[1]=0                //  8 bit column address
                       | FMC_SDCR1_NR_0   // 12 bit row address
                       | FMC_SDCR1_MWID_0 // 16 bit data bus
                       | FMC_SDCR1_NB     //  4 banks
                       | FMC_SDCR1_CAS_1; //  2 cycle CAS latency (F<133MHz)
    
    #ifdef SYSCLK_FREQ_180MHz
    //One SDRAM clock cycle is 11.1ns
    //Some bits in SDTR[1] are don't care, and the have to be set in SDTR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (66.6ns>63ns)
                       | (2-1)<<20;       // 2 cycle TRP  (22.2ns>15ns)
    FMC_Bank5_6->SDTR[1]=(2-1)<<0         // 2 cycle TMRD
                       | (7-1)<<4         // 7 cycle TXSR (77.7ns>70ns)
                       | (4-1)<<8         // 4 cycle TRAS (44.4ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (22.2ns>15ns)
    #elif defined(SYSCLK_FREQ_168MHz)
    //One SDRAM clock cycle is 11.9ns
    //Some bits in SDTR[1] are don't care, and the have to be set in SDTR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (71.4ns>63ns)
                       | (2-1)<<20;       // 2 cycle TRP  (23.8ns>15ns)
    FMC_Bank5_6->SDTR[1]=(2-1)<<0         // 2 cycle TMRD
                       | (6-1)<<4         // 6 cycle TXSR (71.4ns>70ns)
                       | (4-1)<<8         // 4 cycle TRAS (47.6ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (23.8ns>15ns)
    #else
    #error No SDRAM timings for this clock
    #endif

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB2   // Enable bank 2
                       | 1;               // MODE=001 clock enabled
    sdramCommandWait();
    
    //ST and SDRAM datasheet agree a 100us delay is required here.
    delayUs(100);

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB2   // Enable bank 2
                       | 2;               // MODE=010 precharge all command
    sdramCommandWait();
    
    FMC_Bank5_6->SDCMR=  (8-1)<<5         // NRFS=8 SDRAM datasheet says
                                          // "at least two AUTO REFRESH cycles"
                       | FMC_SDCMR_CTB2   // Enable bank 2
                       | 3;               // MODE=011 auto refresh
    sdramCommandWait();

    FMC_Bank5_6->SDCMR=0x220<<9           // MRD=0x220:CAS latency=2 burst len=1
                       | FMC_SDCMR_CTB2   // Enable bank 2
                       | 4;               // MODE=100 load mode register
    sdramCommandWait();

    // 64ms/4096=15.625us
    #ifdef SYSCLK_FREQ_180MHz
    //15.625us*90MHz=1406-20=1386
    FMC_Bank5_6->SDRTR=1386<<1;
    #elif defined(SYSCLK_FREQ_168MHz)
    //15.625us*84MHz=1312-20=1292
    FMC_Bank5_6->SDRTR=1292<<1;
    #else
    #error No refresh timings for this clock
    #endif
}

void IRQbspInit()
{
    //If using SDRAM GPIOs are enabled by configureSdram(), else enable them here
    #ifndef __ENABLE_XRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;
    RCC_SYNC();
    #endif //__ENABLE_XRAM
    
    using namespace interfaces;
    spi1::sck::mode(Mode::ALTERNATE);
    spi1::sck::alternateFunction(5);
    spi1::miso::mode(Mode::ALTERNATE);
    spi1::miso::alternateFunction(5);
    spi1::mosi::mode(Mode::ALTERNATE);
    spi1::mosi::alternateFunction(5);
    
    spi2::sck::mode(Mode::ALTERNATE);
    spi2::sck::alternateFunction(5);
    spi2::miso::mode(Mode::ALTERNATE);
    spi2::miso::alternateFunction(5);
    spi2::mosi::mode(Mode::ALTERNATE);
    spi2::mosi::alternateFunction(5);
    
    i2c::scl::mode(Mode::ALTERNATE_OD);
    i2c::scl::alternateFunction(4);
    i2c::sda::mode(Mode::ALTERNATE_OD);
    i2c::sda::alternateFunction(4);
    
    uart4::rx::mode(Mode::ALTERNATE);
    uart4::rx::alternateFunction(8);
    uart4::tx::mode(Mode::ALTERNATE);
    uart4::tx::alternateFunction(8);
    
    can::rx::mode(Mode::ALTERNATE);
    can::rx::alternateFunction(9);
    can::tx::mode(Mode::ALTERNATE);
    can::tx::alternateFunction(9);
    
    using namespace sensors;
    adis16405::cs::mode(Mode::OUTPUT);
    adis16405::cs::high();
    adis16405::nrst::mode(Mode::OUTPUT);
    adis16405::nrst::high();
    adis16405::ckIn::mode(Mode::ALTERNATE);
    adis16405::ckIn::alternateFunction(2);
    adis16405::dio1::mode(Mode::INPUT);    
    
    ad7994::ab::mode(Mode::INPUT);
    ad7994::nconvst::mode(Mode::OUTPUT);
    
    max21105::cs::mode(Mode::OUTPUT);
    max21105::cs::high();
    
    mpu9250::cs::mode(Mode::OUTPUT);
    mpu9250::cs::high();
    
    ms5803::cs::mode(Mode::OUTPUT);
    ms5803::cs::high();
    
    using namespace actuators;
    hbridgel::ena::mode(Mode::OUTPUT);
    hbridgel::ena::low();
    hbridgel::in::mode(Mode::ALTERNATE);
    hbridgel::in::alternateFunction(2);
    hbridgel::csens::mode(Mode::INPUT_ANALOG);
    
    hbridger::ena::mode(Mode::OUTPUT);
    hbridger::ena::low();
    hbridger::in::mode(Mode::ALTERNATE);
    hbridger::in::alternateFunction(2);
    hbridger::csens::mode(Mode::INPUT_ANALOG);
    
    InAir9B::cs::mode(Mode::OUTPUT);
    InAir9B::cs::high();    
    //NOTE: in the InAir9B datasheet is specified that the nRSR line should be
    //on hi-Z state when idle, thus we set the gpio as open drain
    InAir9B::nrst::mode(Mode::OPEN_DRAIN);
    InAir9B::nrst::high();
    InAir9B::dio0::mode(Mode::INPUT);
    InAir9B::dio1::mode(Mode::INPUT);
    InAir9B::dio2::mode(Mode::INPUT);
    InAir9B::dio3::mode(Mode::INPUT);
    
    _led::mode(Mode::OUTPUT);    
// Removed led blink to speed up boot
//     ledOn();
//     delayMs(100);
//     ledOff();
    DefaultConsole::instance().IRQset(
        STM32SerialBase::get<defaultSerialTxPin,defaultSerialRxPin,
        defaultSerialRtsPin,defaultSerialCtsPin>(
            defaultSerial,defaultSerialSpeed,
            defaultSerialFlowctrl,defaultSerialDma));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    intrusive_ref_ptr<DevFs> devFs = basicFilesystemSetup(SDIODriver::instance());
    devFs->addDevice("gps", intrusive_ref_ptr<Device>(
        new STM32Serial(2,115200,
        Gpio<GPIOA_BASE,2>::getPin(),Gpio<GPIOA_BASE,3>::getPin())));
    devFs->addDevice("radio", intrusive_ref_ptr<Device>(
        new STM32Serial(3,115200,
        Gpio<GPIOB_BASE,10>::getPin(),Gpio<GPIOB_BASE,11>::getPin())));
    #endif //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

/**
 * For safety reasons, we never want the homeone to shutdown.
 * When requested to shutdown, we reboot instead.
 */
void shutdown()
{
    reboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();
    IRQsystemReboot();
}

} //namespace miosix
