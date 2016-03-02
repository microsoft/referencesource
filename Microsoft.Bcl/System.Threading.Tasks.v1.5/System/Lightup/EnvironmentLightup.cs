using System;

namespace System
{
    internal class EnvironmentLightup : Lightup
    {
        public static readonly EnvironmentLightup Instance = new EnvironmentLightup();
        private volatile bool _failedToGetProcessorCount = false;
        private volatile PlatformID _platform = PlatformID.Unknown;
        private Delegate _getProcessorCount;
        private Delegate _getOSVersion;

        public EnvironmentLightup()
            : base(typeof(Environment))
        {
        }

        protected override object GetInstance()
        {
            return null;
        }

        public int ProcessorCount
        {
            get
            {
                if (!_failedToGetProcessorCount)
                {
                    // We don't cache it
                    int count;
                    if (TryGetProcessorCount(out count))
                        return count;

                    _failedToGetProcessorCount = true;
                }

                // We can't retrieve processor count, assume 1
                return 1;                
            }
        }

        private PlatformID Platform
        {
            get
            {
                if (_platform == PlatformID.Unknown)
                {
                    object operatingSystem = Get<object>(ref _getOSVersion, "OSVersion");

                    var lightup = new OperatingSystemLightup(operatingSystem);

                    _platform = lightup.Platform;
                }

                return _platform;
            }
        }

        private bool TryGetProcessorCount(out int count)
        {
            // ProcessorCount on Windows Phone 7.x is critical, which prevents us from retrieving it. 
            // Therefore to avoid the first-chance exception showing to the user (as Phone 7.x does 
            // not have "Just My Code"), we assume that if running under CE, we're not going to be 
            // able to retrieve the processor count. 
            // NOTE: WinCE will still be returned to a Phone 7.x under Phone 8, even though it's running 
            // on a WinNT kernal, which is fine, we don't want to the application see a behavior change in 
            // this case.
            if (Platform == PlatformID.WinCE)
            {
                count = 0;
                return false;
            }

            return TryGet<int>(ref _getProcessorCount, "ProcessorCount", out count);
        }
    }
}
