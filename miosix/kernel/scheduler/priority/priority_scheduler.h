/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
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

#include "config/miosix_settings.h"
#include "priority_scheduler_types.h"
#include "kernel/kernel.h"

#ifdef SCHED_TYPE_PRIORITY

namespace miosix {

/**
 * \internal
 * Priority scheduler.
 */
class PriorityScheduler
{
public:
    /**
     * \internal
     * Add a new thread to the scheduler.
     * This is called when a thread is created.
     * \param thread a pointer to a valid thread instance.
     * The behaviour is undefined if a thread is added multiple timed to the
     * scheduler, or if thread is NULL.
     * \param priority the priority of the new thread.
     * Priority must be a positive value.
     * Note that the meaning of priority is scheduler specific.
     */
    static bool PKaddThread(Thread *thread, PrioritySchedulerPriority priority);

    /**
     * \internal
     * \return true if thread exists, false if does not exist or has been
     * deleted. A joinable thread is considered existing until it has been
     * joined, even if it returns from its entry point (unless it is detached
     * and terminates).
     *
     * Can be called both with the kernel paused and with interrupts disabled.
     */
    static bool PKexists(Thread *thread);

    /**
     * \internal
     * Called when there is at least one dead thread to be removed from the
     * scheduler
     */
    static void PKremoveDeadThreads();

    /**
     * \internal
     * Set the priority of a thread.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be changed.
     * \param newPriority new thread priority.
     * Priority must be a positive value.
     */
    static void PKsetPriority(Thread *thread,
            PrioritySchedulerPriority newPriority);

    /**
     * \internal
     * Get the priority of a thread. Must be callable also with kernel paused
     * or IRQ disabled.
     * Note that the meaning of priority is scheduler specific.
     * \param thread thread whose priority needs to be queried.
     * \return the priority of thread.
     */
    static PrioritySchedulerPriority getPriority(Thread *thread)
    {
        return thread->schedData.priority;
    }

    /**
     * \internal
     * This is called before the kernel is started to by the kernel. The given
     * thread is the idle thread, to be run all the times where no other thread
     * can run.
     */
    static void IRQsetIdleThread(Thread *idleThread);

    /**
     * \internal
     * This member function is called by the kernel every time a thread changes
     * its running status. For example when a thread become sleeping, waiting,
     * deleted or if it exits the sleeping or waiting status
     */
    static void IRQwaitStatusHook(Thread* t) {}

    /**
     * \internal
     * This function is used only by the kernel code to run the scheduler.
     * It finds the next thread in READY status. If the kernel is paused,
     * does nothing. It's behaviour is to modify the global variable
     * miosix::runningThread which always points to the currently running thread.
     */
    static void IRQrunScheduler();
    
    /**
     * \internal
     * \return the next scheduled preemption set by the scheduler
     * In case no preemption is set returns numeric_limits<long long>::max()
     */
    static long long IRQgetNextPreemption();

private:

    ///\internal Vector of lists of threads, there's one list for each priority
    ///Each list s a circular list.
    static Thread *threadList[PRIORITY_MAX];

    ///\internal idle thread
    static Thread *idle;
};

} //namespace miosix

#endif //SCHED_TYPE_PRIORITY
