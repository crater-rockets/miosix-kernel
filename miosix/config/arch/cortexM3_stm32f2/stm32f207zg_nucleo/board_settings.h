/***************************************************************************
 *   Copyright (C) 2012-2022 by Terraneo Federico                          *
 *   Copyright (C) 2023, 2024 by Daniele Cattaneo                          *
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

#include "interfaces-impl/hwmapping.h"

/**
 * \internal
 * Versioning for board_settings.h for out of git tree projects
 */
#define BOARD_SETTINGS_VERSION 300

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1.5KB) and the
/// STM32F207ZG has 128KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Serial port
/// The ST-Link serial adapter in Nucleo 144 boards is connected to PD8/9
const unsigned int defaultSerial=3;
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
const bool defaultSerialDma=true;
using defaultSerialTxPin = serial::tx;
using defaultSerialRxPin = serial::rx;
using defaultSerialRtsPin = unused::pd12; //unused
using defaultSerialCtsPin = unused::pd11; //unused

///\def STDOUT_REDIRECTED_TO_DCC
///If defined, stdout is redirected to the debug communication channel, and
///will be printed if OpenOCD is connected. If not defined, stdout will be
///redirected throug USART1, as usual.
//#define STDOUT_REDIRECTED_TO_DCC

//SD card driver
static const unsigned char sdVoltage=33; //Board powered @ 3.3V
//#define SD_ONE_BIT_DATABUS //Use fast 4 bit bus

/**
 * \}
 */

} //namespace miosix
