// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using Microsoft.Win32;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Diagnostics.Tracing;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Threading.NetCore
{
    // TimerQueue maintains a list of active timers in this AppDomain.  We use a single native timer, supplied by the VM,
    // to schedule all managed timers in the AppDomain.
    //
    // Perf assumptions:  We assume that timers are created and destroyed frequently, but rarely actually fire.
    // There are roughly two types of timer:
    //
    //  - timeouts for operations.  These are created and destroyed very frequently, but almost never fire, because
    //    the whole point is that the timer only fires if something has gone wrong.
    //
    //  - scheduled background tasks.  These typically do fire, but they usually have quite long durations.
    //    So the impact of spending a few extra cycles to fire these is negligible.
    //
    // Because of this, we want to choose a data structure with very fast insert and delete times, and we can live
    // with linear traversal times when firing timers.  However, we still want to minimize the number of timers
    // we need to traverse while doing the linear walk: in cases where we have lots of long-lived timers as well as
    // lots of short-lived timers, when the short-lived timers fire, they incur the cost of walking the long-lived ones.
    //
    // The data structure we've chosen is an unordered doubly-linked list of active timers.  This gives O(1) insertion
    // and removal, and O(N) traversal when finding expired timers.  We maintain two such lists: one for all of the
    // timers that'll next fire within a certain threshold, and one for the rest.
    //
    // Note that all instance methods of this class require that the caller hold a lock on the TimerQueue instance.
    // We partition the timers across multiple TimerQueues, each with its own lock and set of short/long lists,
    // in order to minimize contention when lots of threads are concurrently creating and destroying timers often.
    internal class TimerQueue
    {
        #region Shared TimerQueue instances

        public static TimerQueue[] Instances { get; } = CreateTimerQueues();

        private TimerQueue(int id)
        {
            m_id = id;
        }

        private static TimerQueue[] CreateTimerQueues()
        {
            var queues = new TimerQueue[Environment.ProcessorCount];
            for (int i = 0; i < queues.Length; i++)
            {
                queues[i] = new TimerQueue(i);
            }
            return queues;
        }

        #endregion

        #region interface to native per-AppDomain timer

        private static int TickCount
        {
            [SecuritySafeCritical]
            get
            {
#if !FEATURE_PAL
                // We need to keep our notion of time synchronized with the calls to SleepEx that drive
                // the underlying native timer.  In Win8, SleepEx does not count the time the machine spends
                // sleeping/hibernating.  Environment.TickCount (GetTickCount) *does* count that time,
                // so we will get out of sync with SleepEx if we use that method.
                //
                // So, on Win8, we use QueryUnbiasedInterruptTime instead; this does not count time spent
                // in sleep/hibernate mode.
                if (Environment.IsWindows8OrAbove)
                {
                    ulong time100ns;

                    bool result = Win32Native.QueryUnbiasedInterruptTime(out time100ns);
                    if (!result)
                        throw Marshal.GetExceptionForHR(Marshal.GetLastWin32Error());

                    // convert to 100ns to milliseconds, and truncate to 32 bits.
                    return (int)(uint)(time100ns / 10000);
                }
                else
#endif
                {
                    return Environment.TickCount;
                }
            }
        }

        private readonly int m_id; // TimerQueues[m_id] == this
        [SecurityCritical]
        private Threading.TimerQueue.AppDomainTimerSafeHandle m_appDomainTimer;

        private bool m_isAppDomainTimerScheduled;
        private int m_currentAppDomainTimerStartTicks;
        private uint m_currentAppDomainTimerDuration;

        [SecuritySafeCritical]
        private bool EnsureAppDomainTimerFiresBy(uint requestedDuration)
        {
            // The VM's timer implementation does not work well for very long-duration timers.
            // See kb 950807.
            // So we'll limit our native timer duration to a "small" value.
            // This may cause us to attempt to fire timers early, but that's ok - 
            // we'll just see that none of our timers has actually reached its due time,
            // and schedule the native timer again.
            const uint maxPossibleDuration = 0x0fffffff;
            uint actualDuration = Math.Min(requestedDuration, maxPossibleDuration);

            if (m_isAppDomainTimerScheduled)
            {
                uint elapsed = (uint)(TickCount - m_currentAppDomainTimerStartTicks);
                if (elapsed >= m_currentAppDomainTimerDuration)
                    return true; //the timer's about to fire

                uint remainingDuration = m_currentAppDomainTimerDuration - elapsed;
                if (actualDuration >= remainingDuration)
                    return true; //the timer will fire earlier than this request
            }

            // If Pause is underway then do not schedule the timers
            // A later update during resume will re-schedule
            if (m_pauseTicks != 0)
            {
                Contract.Assert(!m_isAppDomainTimerScheduled);
                Contract.Assert(m_appDomainTimer == null);
                return true;
            }

            if (m_appDomainTimer == null || m_appDomainTimer.IsInvalid)
            {
                Contract.Assert(!m_isAppDomainTimerScheduled);
                Contract.Assert(m_id >= 0 && m_id < Instances.Length && this == Instances[m_id]);

                m_appDomainTimer = Threading.TimerQueue.CreateAppDomainTimer(actualDuration, m_id);
                if (!m_appDomainTimer.IsInvalid)
                {
                    m_isAppDomainTimerScheduled = true;
                    m_currentAppDomainTimerStartTicks = TickCount;
                    m_currentAppDomainTimerDuration = actualDuration;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (Threading.TimerQueue.ChangeAppDomainTimer(m_appDomainTimer, actualDuration))
                {
                    m_isAppDomainTimerScheduled = true;
                    m_currentAppDomainTimerStartTicks = TickCount;
                    m_currentAppDomainTimerDuration = actualDuration;
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        // The VM calls this when a native timer fires.
        [SecuritySafeCritical]
        internal static void AppDomainTimerCallback(int id)
        {
            Contract.Assert(id >= 0 && id < Instances.Length && Instances[id].m_id == id);
            Instances[id].FireNextTimers();
        }

        #endregion

        #region Firing timers

        // The two lists of timers that are part of this TimerQueue.  They conform to a single guarantee:
        // no timer in m_longTimers has an absolute next firing time <= m_currentAbsoluteThreshold.
        // That way, when FireNextTimers is invoked, we always process the short list, and we then only
        // process the long list if the current time is greater than m_currentAbsoluteThreshold (or
        // if the short list is now empty and we need to process the long list to know when to next
        // invoke FireNextTimers).
        private TimerQueueTimer m_shortTimers;
        private TimerQueueTimer m_longTimers;

        // The current threshold, an absolute time where any timers scheduled to go off at or
        // before this time must be queued to the short list.
        private int m_currentAbsoluteThreshold = ShortTimersThresholdMilliseconds;

        // Default threshold that separates which timers target m_shortTimers vs m_longTimers. The threshold
        // is chosen to balance the number of timers in the small list against the frequency with which
        // we need to scan the long list.  It's thus somewhat arbitrary and could be changed based on
        // observed workload demand. The larger the number, the more timers we'll likely need to enumerate
        // every time the appdomain timer fires, but also the more likely it is that when it does we won't
        // need to look at the long list because the current time will be <= m_currentAbsoluteThreshold.
        private const int ShortTimersThresholdMilliseconds = 333;

        // Time when Pause was called
        private volatile int m_pauseTicks = 0;

        [SecurityCritical]
        internal static void PauseAll()
        {
            foreach (TimerQueue queue in Instances)
            {
                queue.Pause();
            }
        }

        [SecurityCritical]
        internal static void ResumeAll()
        {
            foreach (TimerQueue queue in Instances)
            {
                queue.Resume();
            }
        }

        [SecurityCritical]
        internal void Pause()
        {
            lock (this)
            {
                // Delete the native timer so that no timers are fired in the Pause zone
                if (m_appDomainTimer != null && !m_appDomainTimer.IsInvalid)
                {
                    m_appDomainTimer.Dispose();
                    m_appDomainTimer = null;
                    m_isAppDomainTimerScheduled = false;
                    m_pauseTicks = TickCount;
                }
            }
        }

        [SecurityCritical]
        internal void Resume()
        {
            //
            // Update timers to adjust their due-time to accomodate Pause/Resume
            //
            lock (this)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    int pauseTicks = m_pauseTicks;
                    m_pauseTicks = 0; // Set this to 0 so that now timers can be scheduled

                    int nowTicks = TickCount;
                    int pauseDurationTicks = nowTicks - pauseTicks;

                    bool haveTimerToSchedule = false;
                    uint nextAppDomainTimerDuration = uint.MaxValue;

                    TimerQueueTimer timer = m_shortTimers;
                    for (int listNum = 0; listNum < 2; listNum++) // short == 0, long == 1
                    {
                        while (timer != null)
                        {
                            Contract.Assert(timer.m_dueTime != Timeout.UnsignedInfinite);

                            // Save off the next timer to examine, in case our examination of this timer results
                            // in our deleting or moving it; we'll continue after with this saved next timer.
                            TimerQueueTimer next = timer.m_next;

                            uint elapsed; // How much of the timer dueTime has already elapsed

                            // Timers started before the paused event has to be sufficiently delayed to accomodate 
                            // for the Pause time. However, timers started after the Paused event shouldnt be adjusted. 
                            // E.g. ones created by the app in its Activated event should fire when it was designated.
                            // The Resumed event which is where this routine is executing is after this Activated and hence 
                            // shouldn't delay this timer

                            if (pauseDurationTicks <= (nowTicks - timer.m_startTicks))
                                elapsed = (uint)(pauseTicks - timer.m_startTicks);
                            else
                                elapsed = (uint)(nowTicks - timer.m_startTicks);

                            // Handling the corner cases where a Timer was already due by the time Resume is happening,
                            // We shouldn't delay those timers. 
                            // Example is a timer started in App's Activated event with a very small duration
                            timer.m_dueTime = (timer.m_dueTime > elapsed) ? timer.m_dueTime - elapsed : 0;
                            timer.m_startTicks = nowTicks; // re-baseline

                            if (timer.m_dueTime < nextAppDomainTimerDuration)
                            {
                                haveTimerToSchedule = true;
                                nextAppDomainTimerDuration = timer.m_dueTime;
                            }

                            if (!timer.m_short && timer.m_dueTime <= ShortTimersThresholdMilliseconds)
                            {
                                MoveTimerToCorrectList(timer, shortList: true);
                            }

                            timer = next;
                        }

                        // Switch to process the long list if necessary.
                        if (listNum == 0)
                        {
                            // Determine how much time remains between now and the current threshold.  If time remains,
                            // we can skip processing the long list.  We use > rather than >= because, although we
                            // know that if remaining == 0 no timers in the long list will need to be fired, we
                            // don't know without looking at them when we'll need to call FireNextTimers again.  We
                            // could in that case just set the next appdomain firing to 1, but we may as well just iterate the
                            // long list now; otherwise, most timers created in the interim would end up in the long
                            // list and we'd likely end up paying for another invocation of FireNextTimers that could
                            // have been delayed longer (to whatever is the current minimum in the long list).
                            int remaining = m_currentAbsoluteThreshold - nowTicks;
                            if (remaining > 0)
                            {
                                if (m_shortTimers == null && m_longTimers != null)
                                {
                                    // We don't have any short timers left and we haven't examined the long list,
                                    // which means we likely don't have an accurate nextAppDomainTimerDuration.
                                    // But we do know that nothing in the long list will be firing before or at m_currentAbsoluteThreshold,
                                    // so we can just set nextAppDomainTimerDuration to the difference between then and now.
                                    nextAppDomainTimerDuration = (uint)remaining + 1;
                                    haveTimerToSchedule = true;
                                }
                                break;
                            }

                            // Switch to processing the long list.
                            timer = m_longTimers;

                            // Now that we're going to process the long list, update the current threshold.
                            m_currentAbsoluteThreshold = nowTicks + ShortTimersThresholdMilliseconds;
                        }
                    }

                    if (haveTimerToSchedule)
                    {
                        EnsureAppDomainTimerFiresBy(nextAppDomainTimerDuration);
                    }
                }
            }
        }

        // Used by FireNextTimers() to record inside the lock which timers should be queued to fire, so that they may be queued
        // after exiting the lock to avoid unnecessary lock contention in TimerQueueTimer.Fire(). The AppDomain's timer is
        // non-repeating, but it is rescheduled inside the lock before recorded timers would be queued, so it is possible for
        // two calls to FireNextTimers() to be running concurrently on the same TimerQueue instance. A thread-static list avoids
        // the need for further synchronization. This change is specific to NetFx, NetCore does not need the change.
        [ThreadStatic]
        private static List<TimerQueueTimer> t_timersToQueueToFire;

        // Fire any timers that have expired, and update the native timer to schedule the rest of them.
        // We're in a thread pool work item here, and if there are multiple timers to be fired, we want
        // to queue all but the first one.  The first may can then be invoked synchronously or queued,
        // a task left up to our caller, which might be firing timers from multiple queues.
        [SecuritySafeCritical]
        private void FireNextTimers()
        {
            // We fire the first timer on this thread; any other timers that need to be fired
            // are queued to the ThreadPool.
            TimerQueueTimer timerToFireOnThisThread = null;

            // Set up a list that will contain timers that need to be queued to fire
            List<TimerQueueTimer> timersToQueueToFire = t_timersToQueueToFire;
            Contract.Assert(timersToQueueToFire == null || timersToQueueToFire.Count == 0);
            if (timersToQueueToFire == null)
            {
                t_timersToQueueToFire = timersToQueueToFire = new List<TimerQueueTimer>();
            }

            lock (this)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    // Since we got here, that means our previous appdomain timer has fired.
                    m_isAppDomainTimerScheduled = false;
                    bool haveTimerToSchedule = false;
                    uint nextAppDomainTimerDuration = uint.MaxValue;

                    int nowTicks = TickCount;

                    // Sweep through the "short" timers.  If the current tick count is greater than
                    // the current threshold, also sweep through the "long" timers.  Finally, as part
                    // of sweeping the long timers, move anything that'll fire within the next threshold
                    // to the short list.  It's functionally ok if more timers end up in the short list
                    // than is truly necessary (but not the opposite).
                    TimerQueueTimer timer = m_shortTimers;
                    for (int listNum = 0; listNum < 2; listNum++) // short == 0, long == 1
                    {
                        while (timer != null)
                        {
                            Contract.Assert(timer.m_dueTime != Timeout.UnsignedInfinite, "A timer in the list must have a valid due time.");

                            // Save off the next timer to examine, in case our examination of this timer results
                            // in our deleting or moving it; we'll continue after with this saved next timer.
                            TimerQueueTimer next = timer.m_next;

                            uint elapsed = (uint)(nowTicks - timer.m_startTicks);
                            int remaining = (int)timer.m_dueTime - (int)elapsed;
                            if (remaining <= 0)
                            {
                                // Timer is ready to fire.

                                if (timer.m_period != Timeout.UnsignedInfinite)
                                {
                                    // This is a repeating timer; schedule it to run again.

                                    // Discount the extra amount of time that has elapsed since the previous firing time to
                                    // prevent timer ticks from drifting.  If enough time has already elapsed for the timer to fire
                                    // again, meaning the timer can't keep up with the short period, have it fire 1 ms from now to
                                    // avoid spinning without a delay.
                                    timer.m_startTicks = nowTicks;
                                    uint elapsedForNextDueTime = elapsed - timer.m_dueTime;
                                    timer.m_dueTime = (elapsedForNextDueTime < timer.m_period) ?
                                        timer.m_period - elapsedForNextDueTime :
                                        1;

                                    // Update the appdomain timer if this becomes the next timer to fire.
                                    if (timer.m_dueTime < nextAppDomainTimerDuration)
                                    {
                                        haveTimerToSchedule = true;
                                        nextAppDomainTimerDuration = timer.m_dueTime;
                                    }

                                    // Validate that the repeating timer is still on the right list.  It's likely that
                                    // it started in the long list and was moved to the short list at some point, so
                                    // we now want to move it back to the long list if that's where it belongs. Note that
                                    // if we're currently processing the short list and move it to the long list, we may
                                    // end up revisiting it again if we also enumerate the long list, but we will have already
                                    // updated the due time appropriately so that we won't fire it again (it's also possible
                                    // but rare that we could be moving a timer from the long list to the short list here,
                                    // if the initial due time was set to be long but the timer then had a short period).
                                    bool targetShortList = (nowTicks + timer.m_dueTime) - m_currentAbsoluteThreshold <= 0;
                                    if (timer.m_short != targetShortList)
                                    {
                                        MoveTimerToCorrectList(timer, targetShortList);
                                    }
                                }
                                else
                                {
                                    // Not repeating; remove it from the queue
                                    DeleteTimer(timer);
                                }

                                // If this is the first timer, we'll fire it on this thread (after processing
                                // all others). Otherwise, queue it to the ThreadPool.
                                if (timerToFireOnThisThread == null)
                                {
                                    timerToFireOnThisThread = timer;
                                }
                                else
                                {
                                    // Don't queue the timer to the thread pool here. TimerQueueTimer.Fire() acquires the same lock
                                    // that is already held by this method, and leads to unnecessary lock contention. Record the
                                    // timer and it will be queued to fire after exiting the lock.
                                    timersToQueueToFire.Add(timer);
                                }
                            }
                            else
                            {
                                // This timer isn't ready to fire.  Update the next time the native timer fires if necessary,
                                // and move this timer to the short list if its remaining time is now at or under the threshold.

                                if (remaining < nextAppDomainTimerDuration)
                                {
                                    haveTimerToSchedule = true;
                                    nextAppDomainTimerDuration = (uint)remaining;
                                }

                                if (!timer.m_short && remaining <= ShortTimersThresholdMilliseconds)
                                {
                                    MoveTimerToCorrectList(timer, shortList: true);
                                }
                            }

                            timer = next;
                        }

                        // Switch to process the long list if necessary.
                        if (listNum == 0)
                        {
                            // Determine how much time remains between now and the current threshold.  If time remains,
                            // we can skip processing the long list.  We use > rather than >= because, although we
                            // know that if remaining == 0 no timers in the long list will need to be fired, we
                            // don't know without looking at them when we'll need to call FireNextTimers again.  We
                            // could in that case just set the next appdomain firing to 1, but we may as well just iterate the
                            // long list now; otherwise, most timers created in the interim would end up in the long
                            // list and we'd likely end up paying for another invocation of FireNextTimers that could
                            // have been delayed longer (to whatever is the current minimum in the long list).
                            int remaining = m_currentAbsoluteThreshold - nowTicks;
                            if (remaining > 0)
                            {
                                if (m_shortTimers == null && m_longTimers != null)
                                {
                                    // We don't have any short timers left and we haven't examined the long list,
                                    // which means we likely don't have an accurate nextAppDomainTimerDuration.
                                    // But we do know that nothing in the long list will be firing before or at m_currentAbsoluteThreshold,
                                    // so we can just set nextAppDomainTimerDuration to the difference between then and now.
                                    nextAppDomainTimerDuration = (uint)remaining + 1;
                                    haveTimerToSchedule = true;
                                }
                                break;
                            }

                            // Switch to processing the long list.
                            timer = m_longTimers;

                            // Now that we're going to process the long list, update the current threshold.
                            m_currentAbsoluteThreshold = nowTicks + ShortTimersThresholdMilliseconds;
                        }
                    }

                    // If we still have scheduled timers, update the appdomain timer to ensure it fires
                    // in time for the next one in line.
                    if (haveTimerToSchedule)
                    {
                        EnsureAppDomainTimerFiresBy(nextAppDomainTimerDuration);
                    }
                }
            }

            // Queue timers outside the lock to avoid unnecessary lock contention with TimerQueueTimer.Fire()
            if (timersToQueueToFire.Count != 0)
            {
                foreach (TimerQueueTimer timer in timersToQueueToFire)
                {
                    // NetCore calls ThreadPool.UnsafeQueueUserWorkItemInternal(), which was added as part of
                    // https://github.com/dotnet/coreclr/pull/20387 and has not been ported to NetFx
                    ThreadPool.UnsafeQueueCustomWorkItem(timer, forceGlobal: true);
                }
                timersToQueueToFire.Clear();
            }

            // Fire the user timer outside of the lock!
            timerToFireOnThisThread?.Fire();
        }

        #endregion

        #region Queue implementation

        public bool UpdateTimer(TimerQueueTimer timer, uint dueTime, uint period)
        {
            int nowTicks = TickCount;

            // The timer can be put onto the short list if it's next absolute firing time
            // is <= the current absolute threshold.
            int absoluteDueTime = (int)(nowTicks + dueTime);
            bool shouldBeShort = m_currentAbsoluteThreshold - absoluteDueTime >= 0;

            if (timer.m_dueTime == Timeout.UnsignedInfinite)
            {
                // If the timer wasn't previously scheduled, now add it to the right list.
                timer.m_short = shouldBeShort;
                LinkTimer(timer);
            }
            else if (timer.m_short != shouldBeShort)
            {
                // If the timer was previously scheduled, but this update should cause
                // it to move over the list threshold in either direction, do so.
                UnlinkTimer(timer);
                timer.m_short = shouldBeShort;
                LinkTimer(timer);
            }

            timer.m_dueTime = dueTime;
            timer.m_period = (period == 0) ? Timeout.UnsignedInfinite : period;
            timer.m_startTicks = nowTicks;
            return EnsureAppDomainTimerFiresBy(dueTime);
        }

        public void MoveTimerToCorrectList(TimerQueueTimer timer, bool shortList)
        {
            Contract.Assert(timer.m_dueTime != Timeout.UnsignedInfinite, "Expected timer to be on a list.");
            Contract.Assert(timer.m_short != shortList, "Unnecessary if timer is already on the right list.");

            // Unlink it from whatever list it's on, change its list association, then re-link it.
            UnlinkTimer(timer);
            timer.m_short = shortList;
            LinkTimer(timer);
        }

        private void LinkTimer(TimerQueueTimer timer)
        {
            // Use timer.m_short to decide to which list to add.
            timer.m_next = timer.m_short ? m_shortTimers : m_longTimers;
            if (timer.m_next != null)
            {
                timer.m_next.m_prev = timer;
            }
            timer.m_prev = null;
            if (timer.m_short)
            {
                m_shortTimers = timer;
            }
            else
            {
                m_longTimers = timer;
            }
        }

        private void UnlinkTimer(TimerQueueTimer timer)
        {
            TimerQueueTimer t = timer.m_next;
            if (t != null)
            {
                t.m_prev = timer.m_prev;
            }

            if (m_shortTimers == timer)
            {
                Contract.Assert(timer.m_short);
                m_shortTimers = t;
            }
            else if (m_longTimers == timer)
            {
                Contract.Assert(!timer.m_short);
                m_longTimers = t;
            }

            t = timer.m_prev;
            if (t != null)
            {
                t.m_next = timer.m_next;
            }

            // At this point the timer is no longer in a list, but its next and prev
            // references may still point to other nodes.  UnlinkTimer should thus be
            // followed by something that overwrites those references, either with null
            // if deleting the timer or other nodes if adding it to another list.
        }

        public void DeleteTimer(TimerQueueTimer timer)
        {
            if (timer.m_dueTime != Timeout.UnsignedInfinite)
            {
                UnlinkTimer(timer);
                timer.m_prev = null;
                timer.m_next = null;
                timer.m_dueTime = Timeout.UnsignedInfinite;
                timer.m_period = Timeout.UnsignedInfinite;
                timer.m_startTicks = 0;
                timer.m_short = false;
            }
        }

        #endregion
    }

    // A timer in our TimerQueue.
    internal sealed class TimerQueueTimer : IThreadPoolWorkItem
    {
        // The associated timer queue.
        private readonly TimerQueue m_associatedTimerQueue;

        // All mutable fields of this class are protected by a lock on m_associatedTimerQueue.
        // The first six fields are maintained by TimerQueue.

        // Links to the next and prev timers in the list.
        internal TimerQueueTimer m_next;
        internal TimerQueueTimer m_prev;

        // true if on the short list; otherwise, false.
        internal bool m_short;

        // The time, according to TimerQueue.TickCount, when this timer's current interval started.
        internal int m_startTicks;

        // Timeout.UnsignedInfinite if we are not going to fire.  Otherwise, the offset from m_startTime when we will fire.
        internal uint m_dueTime;

        // Timeout.UnsignedInfinite if we are a single-shot timer.  Otherwise, the repeat interval.
        internal uint m_period;

        // Info about the user's callback
        private readonly TimerCallback m_timerCallback;
        private readonly object m_state;
        private readonly ExecutionContext m_executionContext;

        // When Timer.Dispose(WaitHandle) is used, we need to signal the wait handle only
        // after all pending callbacks are complete.  We set m_canceled to prevent any callbacks that
        // are already queued from running.  We track the number of callbacks currently executing in 
        // m_callbacksRunning.  We set m_notifyWhenNoCallbacksRunning only when m_callbacksRunning
        // reaches zero.
        private int m_callbacksRunning;
        private volatile bool m_canceled;
        private volatile WaitHandle m_notifyWhenNoCallbacksRunning;


        [SecuritySafeCritical]
        internal TimerQueueTimer(TimerCallback timerCallback, object state, uint dueTime, uint period, bool flowExecutionContext, ref StackCrawlMark stackMark)
        {
            m_timerCallback = timerCallback;
            m_state = state;
            m_dueTime = Timeout.UnsignedInfinite;
            m_period = Timeout.UnsignedInfinite;
            if (flowExecutionContext)
            {
                m_executionContext = ExecutionContext.Capture(
                    ref stackMark,
                    ExecutionContext.CaptureOptions.IgnoreSyncCtx | ExecutionContext.CaptureOptions.OptimizeDefaultCase);
            }
            m_associatedTimerQueue = TimerQueue.Instances[Thread.GetCurrentProcessorId() % TimerQueue.Instances.Length];

            // After the following statement, the timer may fire.  No more manipulation of timer state outside of
            // the lock is permitted beyond this point!
            if (dueTime != Timeout.UnsignedInfinite)
                Change(dueTime, period);
        }

        internal bool Change(uint dueTime, uint period)
        {
            bool success;

            lock (m_associatedTimerQueue)
            {
                if (m_canceled)
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic"));

                // prevent ThreadAbort while updating state
                try {} finally
                {
                    m_period = period;

                    if (dueTime == Timeout.UnsignedInfinite)
                    {
                        m_associatedTimerQueue.DeleteTimer(this);
                        success = true;
                    }
                    else
                    {
                        if (FrameworkEventSource.IsInitialized && FrameworkEventSource.Log.IsEnabled(EventLevel.Informational, FrameworkEventSource.Keywords.ThreadTransfer))
                            FrameworkEventSource.Log.ThreadTransferSendObj(this, 1, string.Empty, true);

                        success = m_associatedTimerQueue.UpdateTimer(this, dueTime, period);
                    }
                }
            }

            return success;
        }


        public void Close()
        {
            lock (m_associatedTimerQueue)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    if (!m_canceled)
                    {
                        m_canceled = true;
                        m_associatedTimerQueue.DeleteTimer(this);
                    }
                }
            }
        }


        public bool Close(WaitHandle toSignal)
        {
            bool success;
            bool shouldSignal = false;

            lock (m_associatedTimerQueue)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    if (m_canceled)
                    {
                        success = false;
                    }
                    else
                    {
                        m_canceled = true;
                        m_notifyWhenNoCallbacksRunning = toSignal;
                        m_associatedTimerQueue.DeleteTimer(this);
                        shouldSignal = m_callbacksRunning == 0;
                        success = true;
                    }
                }
            }

            if (shouldSignal)
                SignalNoCallbacksRunning();

            return success;
        }

        [SecurityCritical]
        void IThreadPoolWorkItem.ExecuteWorkItem() => Fire();

        [SecurityCritical]
        void IThreadPoolWorkItem.MarkAborted(ThreadAbortException tae)
        {
        }

        internal void Fire()
        {
            bool canceled = false;

            lock (m_associatedTimerQueue)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    canceled = m_canceled;
                    if (!canceled)
                        m_callbacksRunning++;
                }
            }

            if (canceled)
                return;

            CallCallback();

            bool shouldSignal = false;
            lock (m_associatedTimerQueue)
            {
                // prevent ThreadAbort while updating state
                try {} finally
                {
                    m_callbacksRunning--;
                    if (m_canceled && m_callbacksRunning == 0 && m_notifyWhenNoCallbacksRunning != null)
                        shouldSignal = true;
                }
            }

            if (shouldSignal)
                SignalNoCallbacksRunning();
        }

        [SecuritySafeCritical]
        internal void SignalNoCallbacksRunning()
        {
            Contract.Assert(m_notifyWhenNoCallbacksRunning != null);
            Win32Native.SetEvent(m_notifyWhenNoCallbacksRunning.SafeWaitHandle);
        }

        [SecuritySafeCritical]
        internal void CallCallback()
        {
            if (FrameworkEventSource.IsInitialized && FrameworkEventSource.Log.IsEnabled(EventLevel.Informational, FrameworkEventSource.Keywords.ThreadTransfer))
                FrameworkEventSource.Log.ThreadTransferReceiveObj(this, 1, string.Empty);

            // Call directly if EC flow is suppressed
            ExecutionContext context = m_executionContext;
            if (context == null)
            {
                m_timerCallback(m_state);
            }
            else
            {
                using (context = context.IsPreAllocatedDefault ? context : context.CreateCopy())
                {
                    ContextCallback callback = s_callCallbackInContext;
                    if (callback == null)
                    {
                        s_callCallbackInContext = callback = new ContextCallback(CallCallbackInContext);
                    }

                    ExecutionContext.Run(
                        context,
                        s_callCallbackInContext,
                        this,  // state
                        true); // ignoreSyncCtx
                }
            }
        }

        [SecurityCritical]
        private static ContextCallback s_callCallbackInContext;

        [SecurityCritical]
        private static void CallCallbackInContext(object state)
        {
            TimerQueueTimer t = (TimerQueueTimer)state;
            t.m_timerCallback(t.m_state);
        }
    }
}
