/***************************************************************************
 *   Copyright (C) 2015-2020 by Terraneo Federico                          *
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

#include <utility>
#include <sys/ioctl.h>
#include "interfaces/gpio.h"
#include "interfaces/delays.h"
#include "interfaces/arch_registers.h"
#include "interfaces/bsp.h"
#include "interfaces_private/bsp_private.h"
#include "interfaces/poweroff.h"
#include "config/miosix_settings.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"
#include "drivers/serial.h"
#include "board_settings.h"

using namespace std;

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Configuring GPIO pins of USART2 to the proper alternate function
    using rx = Gpio<GPIOA_BASE,25>;
    using tx = Gpio<GPIOA_BASE,26>;
    rx::alternateFunction('B');
    tx::alternateFunction('B');
    rx::mode(Mode::ALTERNATE_PULL_UP);
    tx::mode(Mode::ALTERNATE);

    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new ATSAMSerial(defaultSerial,defaultSerialSpeed)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    //Passing an empty device won't mount fat32, but will mount romfs and devfs
    basicFilesystemSetup(intrusive_ref_ptr<Device>());
    #endif //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    reboot(); //This board has no shutdown support, so we reboot on shutdown
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    IRQsystemReboot();
}

} //namespace miosix
