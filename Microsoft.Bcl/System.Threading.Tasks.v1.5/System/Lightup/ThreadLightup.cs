using System;
using System.Reflection;
using System.Threading;

namespace System
{
    internal class ThreadLightup : Lightup
    {
        public static readonly ThreadLightup Current = new ThreadLightup();
        private readonly Func<Thread> _thread;
        private Delegate _start;
        private Delegate _threadState;
        private Delegate _yield;
        private Delegate _sleep;
        private Delegate _spinWait;
        private Delegate _isAlive;
        private Delegate _isBackground;

        public ThreadLightup()
            : this(() => Thread.CurrentThread)
        {
        }

        public ThreadLightup(Thread thread)
            : this(() => thread)
        {   
        }

        private ThreadLightup(Func<Thread> getThread)
            : base(typeof(Thread))
        {
            _thread = getThread;
        }

        protected override object GetInstance()
        {
            return _thread();
        }

        public static ThreadLightup Create(Action<object> start)
        {
            Type delegateType = LightupType.ParameterizedThreadStart;
            if (delegateType == null)
                throw new InvalidOperationException();

            // Replace the Action<object> with a ParameterizedThreadStart
            Delegate parameterizedThreadStart = LightupServices.ReplaceWith(start, delegateType);

            Thread thread = Create<Thread>(parameterizedThreadStart);

            return new ThreadLightup(thread);
        }

        public void Start(object parameter)
        {
            Call(ref _start, "Start", parameter);
        }

        public ThreadState ThreadState
        {
            get { return (ThreadState)Get<int>(ref _threadState, "ThreadState"); }
        }

        public void Yield()
        {
            // No-op if it doesn't exist or is not callable
            bool ignored;
            TryCall(ref _yield, "Yield", out ignored);
        }

        public void Sleep(int millisecondsTimeout)
        {
            Call(ref _sleep, "Sleep", millisecondsTimeout);
        }

        public void SpinWait(int iterations)
        {
            Call(ref _spinWait, "SpinWait", iterations);
        }

        public bool IsBackground
        {
            set { Set(ref _isBackground, "IsBackground", value); }
        }

        public bool IsAlive
        {
            get { return Get<bool>(ref _isAlive, "IsAlive"); }
        }
    }

    [Flags]
    internal enum ThreadState
    {
        Running = 0,
        StopRequested = 1,
        SuspendRequested = 2,
        Background = 4,
        Unstarted = 8,
        Stopped = 16,
        WaitSleepJoin = 32,
        Suspended = 64,
        AbortRequested = 128,
        Aborted = 256,
    }
}
