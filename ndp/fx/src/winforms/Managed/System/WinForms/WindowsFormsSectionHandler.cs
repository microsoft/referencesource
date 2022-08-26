
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Forms.WindowsFormsSectionHandler..ctor()")]

namespace System.Windows.Forms {
    using System;
    using System.Diagnostics;
    using System.Configuration;

    public sealed class WindowsFormsSection : ConfigurationSection {
        internal const bool                             JitDebuggingDefault = false;

        private static ConfigurationPropertyCollection  s_properties;
        private static ConfigurationProperty            s_propJitDebugging;

        internal static WindowsFormsSection GetSection() {
            WindowsFormsSection section = null;

            try {
                section = (WindowsFormsSection) System.Configuration.PrivilegedConfigurationManager.GetSection("system.windows.forms");
            }
            catch {
                Debug.Fail("Exception loading config for windows forms");
                section = new WindowsFormsSection();
            }

            return section;
        }

        private static ConfigurationPropertyCollection EnsureStaticPropertyBag() {
            if (s_properties == null) {
                s_propJitDebugging = new ConfigurationProperty("jitDebugging", typeof(bool), JitDebuggingDefault, ConfigurationPropertyOptions.None);

                ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();
                properties.Add(s_propJitDebugging);
                s_properties = properties;
            }

            return s_properties;
        }

        public WindowsFormsSection() {
            EnsureStaticPropertyBag();
        }

        protected override ConfigurationPropertyCollection Properties {
            get {
                return EnsureStaticPropertyBag();
            }
        }

        [ConfigurationProperty("jitDebugging", DefaultValue=JitDebuggingDefault)]
        public bool JitDebugging {
            get {
                return (bool) base[s_propJitDebugging];
            }

            set {
                base[s_propJitDebugging] = value;
            }
        }
    }
}

