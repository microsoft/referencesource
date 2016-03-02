using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Threading
{

    // Workaround for the lack of rooting of timers on Compat Framework/Windows Phone
    internal static class TimerManager
    {
#if NO_TIMER_ROOT
        private static Dictionary<Timer, object> s_rootedTimers = new Dictionary<Timer, object>();
#endif
        public static void Add(Timer timer)
        {
#if NO_TIMER_ROOT
            lock (s_rootedTimers)
            {
                s_rootedTimers.Add(timer, null);
            }
#endif
        }

        public static void Remove(Timer timer)
        {
#if NO_TIMER_ROOT
            lock (s_rootedTimers)
            {
                s_rootedTimers.Remove(timer);
            }
#endif
        }
    }
}
