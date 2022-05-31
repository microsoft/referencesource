using System.Collections.Concurrent;
using System.Runtime.CompilerServices;

namespace System.Threading
{
    internal sealed class ThreadStatic<T>
    {
        private readonly ConcurrentDictionary<Thread, StrongBox<T>> m_tls = new ConcurrentDictionary<Thread, StrongBox<T>>();

        public T Value
        {
            get
            {
                StrongBox<T> sb;
                if (m_tls.TryGetValue(Thread.CurrentThread, out sb)) return sb.Value;
                return default(T);
            }
            set
            {
                var currentThread = Thread.CurrentThread;
                StrongBox<T> sb;
                if (m_tls.TryGetValue(currentThread, out sb))
                {
                    sb.Value = value;
                }
                else
                {
                    // Clean out any stale threads
                    foreach (var pair in m_tls)
                    {
                        ThreadLightup thread = new ThreadLightup(pair.Key);
                        if (!thread.IsAlive) m_tls.TryRemove(pair.Key, out sb);
                    }

                    // Now add this one
                    m_tls.TryAdd(currentThread, new StrongBox<T>(value));
                }
            }
        }
    }
}