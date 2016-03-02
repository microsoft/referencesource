using System;

namespace System
{
    internal class OperatingSystemLightup : Lightup
    {
        private Delegate _platform;
        private readonly object _getOperatingSystem;

        internal OperatingSystemLightup(object operatingSystem)
            : base(LightupType.OperatingSystem)
        {
            _getOperatingSystem = operatingSystem;
        }

        protected override object GetInstance()
        {
            return _getOperatingSystem;
        }

        public PlatformID Platform
        {
            get { return (PlatformID)Get<int>(ref _platform, "Platform"); }
        }
    }

    internal enum PlatformID
    {
        Unknown = -1,
        Win32S = 0,
        Win32Windows = 1,
        Win32NT = 2,
        WinCE = 3,
        Unix = 4,
        Xbox = 5,
        MacOSX = 6,
    }
}
