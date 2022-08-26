//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#if DEBUG

namespace System.ServiceModel.Activation
{
    using System.Diagnostics;
    using System.IO;
    using System.ServiceProcess;
    using Microsoft.Win32;
    using System.Security;
    using System.Threading;
    using System;
    using System.Runtime.Versioning;

    static class DebuggableService
    {
        static bool ReadRegistryFlag(string serviceName, string flagName)
        {
            bool result = false;
            string subKeyName = "SYSTEM\\CurrentControlSet\\Services\\" + serviceName;
            if (subKeyName.Length < 256)
            {
                try
                {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(subKeyName))
                    {
                        if (key != null)
                        {
                            object value = key.GetValue(flagName, 0);
                            if (value is int)
                            {
                                result = ((int)value) != 0;
                            }
                        }
                    }
                }
                catch (IOException) { }
                catch (SecurityException) { }
            }
            return result;
        }

        public static bool DelayStart(string serviceName)
        {
            return ReadRegistryFlag(serviceName, "DelayStart");
        }

    [ResourceConsumption(ResourceScope.Process)]
        public static void WaitForDebugger(string serviceName)
        {
            bool waitForDebugger = ReadRegistryFlag(serviceName, "WaitForDebugger");
            Debug.Print("DebuggableService.DelayStart() serviceName: " + serviceName + " waitForDebugger : " + waitForDebugger);
            if (waitForDebugger)
            {
                for (int sleepCount = 0; sleepCount < 100; sleepCount++)
                {
                    if (ListenerUnsafeNativeMethods.IsDebuggerPresent())
                    {
                        ListenerUnsafeNativeMethods.DebugBreak();
                        break;
                    }
                    if (Debugger.IsAttached)
                    {
                        Debugger.Break();
                        break;
                    }
                    Thread.Sleep(500);
                }
            }
        }
    }
}

#endif
