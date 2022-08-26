//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation.Diagnostics
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Diagnostics;
    using System.ServiceModel.Activation.Diagnostics;        

    static class ListenerPerfCounters
    {
        static readonly string CategoryName = PerformanceCounterStrings.SMSVCHOST.SMSvcHostPerfCounters;
        static PerformanceCounter perfCounterConnectionsAcceptedNamedPipe;
        static PerformanceCounter perfCounterConnectionsAcceptedTcp;
        static PerformanceCounter perfCounterDispatchFailuresNamedPipe;
        static PerformanceCounter perfCounterDispatchFailuresTcp;
        static PerformanceCounter perfCounterProtocolFailuresNamedPipe;
        static PerformanceCounter perfCounterProtocolFailuresTcp;
        static PerformanceCounter perfCounterUrisRegisteredNamedPipe;
        static PerformanceCounter perfCounterUrisRegisteredTcp;
        static PerformanceCounter perfCounterRegistrationsActiveNamedPipe;
        static PerformanceCounter perfCounterRegistrationsActiveTcp;
        static PerformanceCounter perfCounterConnectionsDispatchedNamedPipe;
        static PerformanceCounter perfCounterConnectionsDispatchedTcp;
        static PerformanceCounter perfCounterUrisUnregisteredNamedPipe;
        static PerformanceCounter perfCounterUrisUnregisteredTcp;
        static List<PerformanceCounter> perfList;
        static object syncObject = new object();

        static ListenerPerfCounters()
        {
            if (ListenerConfig.PerformanceCountersEnabled)
            {
                ListenerPerfCounters.AddCounterToList(perfCounterConnectionsDispatchedTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ConnectionsDispatchedTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterConnectionsDispatchedNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ConnectionsDispatchedNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterDispatchFailuresTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.DispatchFailuresTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterDispatchFailuresNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.DispatchFailuresNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterUrisRegisteredTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.UrisRegisteredTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterUrisRegisteredNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.UrisRegisteredNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterUrisUnregisteredTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.UrisUnregisteredTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterUrisUnregisteredNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.UrisUnregisteredNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterRegistrationsActiveTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.RegistrationsActiveTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterRegistrationsActiveNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.RegistrationsActiveNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterConnectionsAcceptedTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ConnectionsAcceptedTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterConnectionsAcceptedNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ConnectionsAcceptedNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterProtocolFailuresTcp = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ProtocolFailuresTcp, string.Empty, PerformanceCounterInstanceLifetime.Global));
                ListenerPerfCounters.AddCounterToList(perfCounterProtocolFailuresNamedPipe = ListenerPerfCounters.GetListenerPerformanceCounter(CategoryName, PerformanceCounterStrings.SMSVCHOST.ProtocolFailuresNamedPipe, string.Empty, PerformanceCounterInstanceLifetime.Global));

                AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(ListenerPerfCounters.ExitOrUnloadEventHandler);
                AppDomain.CurrentDomain.DomainUnload += new EventHandler(ListenerPerfCounters.ExitOrUnloadEventHandler);
                AppDomain.CurrentDomain.ProcessExit += new EventHandler(ListenerPerfCounters.ExitOrUnloadEventHandler);
            }
        }

        static PerformanceCounter GetListenerPerformanceCounter(string categoryName, string perfCounterName, string instanceName, PerformanceCounterInstanceLifetime instanceLifetime)
        {
            return PerformanceCounters.GetPerformanceCounterInternal(categoryName, perfCounterName, instanceName, instanceLifetime);
        }

        static void AddCounterToList(PerformanceCounter counter)
        {
            if (counter != null)
            {
                ListenerPerfCounters.PerformanceList.Add(counter);
            }
        }

        static List<PerformanceCounter> PerformanceList
        {
            get
            {
                if (ListenerPerfCounters.perfList == null)
                {
                    lock (ListenerPerfCounters.syncObject)
                    {
                        if (ListenerPerfCounters.perfList == null)
                        {
                            ListenerPerfCounters.perfList = new List<PerformanceCounter>();
                        }
                    }
                }

                return ListenerPerfCounters.perfList;
            }
        }


        internal static void DecrementRegistrationsActiveNamedPipe()
        {
            ListenerPerfCounters.DecrementCounter(ref perfCounterRegistrationsActiveNamedPipe);
        }

        internal static void DecrementRegistrationsActiveTcp()
        {
            ListenerPerfCounters.DecrementCounter(ref perfCounterRegistrationsActiveTcp);
        }

        internal static void IncrementConnectionsAcceptedNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterConnectionsAcceptedNamedPipe);
        }

        internal static void IncrementConnectionsAcceptedTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterConnectionsAcceptedTcp);
        }

        internal static void IncrementDispatchFailuresNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterDispatchFailuresNamedPipe);
        }

        internal static void IncrementDispatchFailuresTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterDispatchFailuresTcp);
        }

        internal static void IncrementProtocolFailuresNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterProtocolFailuresNamedPipe);
        }
        internal static void IncrementProtocolFailuresTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterProtocolFailuresTcp);
        }

        internal static void IncrementUrisRegisteredNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterUrisRegisteredNamedPipe);
        }

        internal static void IncrementUrisRegisteredTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterUrisRegisteredTcp);
        }

        internal static void IncrementRegistrationsActiveNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterRegistrationsActiveNamedPipe);
        }

        internal static void IncrementRegistrationsActiveTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterRegistrationsActiveTcp);
        }

        internal static void IncrementConnectionsDispatchedNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterConnectionsDispatchedNamedPipe);
        }

        internal static void IncrementConnectionsDispatchedTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterConnectionsDispatchedTcp);
        }

        internal static void IncrementUrisUnregisteredNamedPipe()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterUrisUnregisteredNamedPipe);
        }

        internal static void IncrementUrisUnregisteredTcp()
        {
            ListenerPerfCounters.IncrementCounter(ref perfCounterUrisUnregisteredTcp);
        }

        static void IncrementCounter(ref PerformanceCounter counter)
        {
            if (counter != null)
            {
                try
                {
                    counter.Increment();
                }
#pragma warning suppress 56500 // covered by FxCOP
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    ListenerPerfCounters.TracePerformanceCounterUpdateFailure(counter.InstanceName, counter.CounterName);
                    counter = null;
                }
            }
        }

        static void DecrementCounter(ref PerformanceCounter counter)
        {
            if (counter != null)
            {
                try
                {
                    counter.Decrement();
                }
#pragma warning suppress 56500 // covered by FxCOP
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    ListenerPerfCounters.TracePerformanceCounterUpdateFailure(counter.InstanceName, counter.CounterName);
                    counter = null;
                }
            }
        }

        static internal void TracePerformanceCounterUpdateFailure(string instanceName, string perfCounterName)
        {
            if (DiagnosticUtility.ShouldTraceError)
            {
                ListenerTraceUtility.TraceEvent(
                    TraceEventType.Error,
                    ListenerTraceCode.PerformanceCountersFailedDuringUpdate,
                    SR.GetString(System.ServiceModel.SR.TraceCodePerformanceCountersFailedDuringUpdate,
                        perfCounterName + "::" + instanceName), null);
            }
        }

        static void ExitOrUnloadEventHandler(object sender, EventArgs e)
        {
            List<PerformanceCounter> countersToRemove = null;

            if (ListenerPerfCounters.perfList != null)
            {
                lock (syncObject)
                {
                    if (ListenerPerfCounters.perfList != null)
                    {
                        countersToRemove = ListenerPerfCounters.perfList;
                        ListenerPerfCounters.perfList = null;
                    }
                }
            }

            if (null != countersToRemove)
            {
                foreach (PerformanceCounter counter in countersToRemove)
                {
                    string counterName = counter.CounterName;
                    string categoryName = counter.CategoryName;
                    string instanceName = counter.InstanceName;

                    try
                    {
                        counter.RemoveInstance();
                    }
#pragma warning suppress 56500 // covered by FxCOP
                    catch (Exception exception)
                    {
                        if (Fx.IsFatal(exception))
                        {
                            throw;
                        }

                        ListenerTraceUtility.EventLog.LogEvent(
                            TraceEventType.Error,
                            (ushort)System.Runtime.Diagnostics.EventLogCategory.PerformanceCounter,
                            (uint)System.Runtime.Diagnostics.EventLogEventId.FailedToRemovePerformanceCounter,
                            false,
                            categoryName,
                            counterName,
                            instanceName,
                            exception.ToString());
                    }
                }
            }

        }

    }
}
