/***************************************************************************
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

#include <termios.h>
#include <errno.h>
#include "config/miosix_settings.h"
#include "filesystem/ioctl.h"
#include "rp2040_serial.h"

namespace miosix {

RP2040PL011Serial::RP2040PL011Serial(int number, int baudrate, GpioPin tx, GpioPin rx)
    : Device(Device::TTY), txLowWaterFlag(1), rxQueue(32+baudrate/500)
{
    tx.function(Function::UART); tx.mode(Mode::OUTPUT);
    rx.function(Function::UART); rx.mode(Mode::INPUT);
    commonInit(number,baudrate);
    uart->cr = UART_UARTCR_UARTEN_BITS
             | UART_UARTCR_TXE_BITS
             | UART_UARTCR_RXE_BITS;
}

RP2040PL011Serial::RP2040PL011Serial(int number, int baudrate, GpioPin tx, GpioPin rx,
    GpioPin rts, GpioPin cts) : Device(Device::TTY), txLowWaterFlag(1), rxQueue(32+baudrate/500)
{
    tx.function(Function::UART); tx.mode(Mode::OUTPUT);
    rx.function(Function::UART); rx.mode(Mode::INPUT);
    rts.function(Function::UART); rts.mode(Mode::OUTPUT);
    cts.function(Function::UART); cts.mode(Mode::INPUT);
    commonInit(number,baudrate);
    uart->cr = UART_UARTCR_UARTEN_BITS
             | UART_UARTCR_TXE_BITS
             | UART_UARTCR_RXE_BITS
             | UART_UARTCR_RTSEN_BITS
             | UART_UARTCR_CTSEN_BITS;
}

ssize_t RP2040PL011Serial::readBlock(void *buffer, size_t size, off_t where)
{
    if(size == 0) return 0;
    Lock<FastMutex> lock(rxMutex);
    auto bytes = reinterpret_cast<unsigned char *>(buffer);
    size_t i = 0;
    // Block until we can read the first byte
    rxQueue.get(bytes[i++]);
    // Get bytes as long as there are bytes in the software queue or the
    // hardware FIFO.
    // As the interrupt handler never empties the FIFO unless the line is idle,
    // this also tells us if the line is idle and we should stop.
    while(i<size && (!(uart->fr & UART_UARTFR_RXFE_BITS) || !rxQueue.isEmpty()))
    {
        rxQueue.get(bytes[i++]);
        // Ensure the read interrupts can be serviced to read the next byte.
        // The interrupt routine disables them on sw queue full.
        if(rxQueue.free()>=32) enableAllInterrupts();
    }
    return i;
}

ssize_t RP2040PL011Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    if(size == 0) return 0;
    Lock<FastMutex> lock(txMutex);
    auto bytes = reinterpret_cast<const unsigned char *>(buffer);
    size_t i=0;
    // Clear the low water semaphore in case it has been left set by a previous
    // transfer. Ordinarily the semaphore counter cannot exceed 1 (or 2, see
    // later comments), except if somebody is using IRQwrite() a bit too much,
    // so we completely reset the semaphore to avoid wasting time on spurious
    // wakeups.
    txLowWaterFlag.reset();
    // Start by filling the hardware FIFO.
    while(i<size && !(uart->fr & UART_UARTFR_TXFF_BITS)) uart->dr = bytes[i++];
    while(i<size)
    {
        // Wait for more space in the FIFO to arrive.
        //   There should be at least 16 bytes free in the fifo (as we are
        // configuring the threshold like that) but it's possible there are less
        // or even zero. This happens when the byte just past the interrupt
        // trigger threshold is removed from the FIFO immediately after it is
        // written. In this specific case the semaphore can reach a value of 2
        // if the FIFO is already flushed by now (but this is really unlikely).
        //   As a result we cannot assume there is space in the FIFO after a
        // wakeup here.
        txLowWaterFlag.wait();
        // Fill the FIFO again
        while(i<size && !(uart->fr & UART_UARTFR_TXFF_BITS))
            uart->dr = bytes[i++];
    }
    return size;
}

void RP2040PL011Serial::IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled.
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();
    // Write to the data register directly
    for(int i=0; str[i] != '\0'; i++)
    {
        while(uart->fr & UART_UARTFR_TXFF_BITS) ;
        uart->dr = str[i];
    }
    // Flush
    while(!(uart->fr & UART_UARTFR_TXFE_BITS)) ;
    // We might be tempted to clear the TX interrupt status, but we shouldn't
    // do this as there might be another thread writing to the UART which needs
    // that interrupt to be signalled anyway.
    if(interrupts) fastEnableInterrupts();
}

int RP2040PL011Serial::ioctl(int cmd, void *arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            while(!(uart->fr & UART_UARTFR_TXFE_BITS)) ;
            return 0;
        case IOCTL_TCGETATTR:
            t->c_iflag=IGNBRK | IGNPAR;
            t->c_oflag=0;
            t->c_cflag=CS8;
            t->c_lflag=0;
            return 0;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            //Changing things at runtime unsupported, so do nothing, but don't
            //return error as console_device.h implements some attribute changes
            return 0;
        default:
            return -ENOTTY; //Means the operation does not apply to this descriptor
    }
}

RP2040PL011Serial::~RP2040PL011Serial()
{
    //Disable UART operation
    uart->cr = 0;
    IRQunregisterIrq(irqn, &RP2040PL011Serial::IRQhandleInterrupt, this);
}

void RP2040PL011Serial::commonInit(int number, int baudrate)
{
    switch(number)
    {
        case 0:
            unreset_block_wait(RESETS_RESET_UART0_BITS);
            uart=uart0_hw;
            irqn=UART0_IRQ_IRQn;
            break;
        case 1:
            unreset_block_wait(RESETS_RESET_UART1_BITS);
            uart=uart1_hw;
            irqn=UART1_IRQ_IRQn;
            break;
        default:
            errorHandler(UNEXPECTED);
    }
    IRQregisterIrq(irqn, &RP2040PL011Serial::IRQhandleInterrupt, this);
    uart->ifls = (2<<UART_UARTIFLS_RXIFLSEL_LSB) | (2<<UART_UARTIFLS_TXIFLSEL_LSB);
    enableAllInterrupts();
    //Setup baud rate
    int rate = 16 * baudrate;
    int div = peripheralFrequency / rate;
    int frac = ((rate * 128) / (peripheralFrequency % rate) + 1) / 2;
    uart->ibrd = div;
    uart->fbrd = frac;
    //Line configuration and UART enable
    uart->lcr_h = (3 << UART_UARTLCR_H_WLEN_LSB) | UART_UARTLCR_H_FEN_BITS;
}

void RP2040PL011Serial::IRQhandleInterrupt()
{
    uint32_t flags = uart->mis;
    if(flags & UART_UARTMIS_TXMIS_BITS)
    {
        // Wake up the thread currently writing and clear interrupt status
        txLowWaterFlag.IRQsignal();
        uart->icr = UART_UARTICR_TXIC_BITS;
    }
    if(flags & (UART_UARTMIS_RXMIS_BITS|UART_UARTMIS_RTMIS_BITS))
    {
        // Read enough data to clear the interrupt status,
        // or until the software-side queue is full
        while((uart->mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS))
                && !rxQueue.isFull())
            rxQueue.IRQput(static_cast<unsigned char>(uart->dr));
        // If the sw queue is full, mask RX interrupts temporarily. The
        // device read handler will un-mask them when the queue has some
        // space again. If there was more data to read and hence the interrupt
        // flag was not cleared, un-masking the interrupts causes the immediate
        // reentry in this interrupt handler, which allows to finish the work
        // without losing the line idle status information (which only exists
        // in the interrupt flags).
        if(rxQueue.isFull()) disableRXInterrupts();
    }
}

} // namespace miosix
