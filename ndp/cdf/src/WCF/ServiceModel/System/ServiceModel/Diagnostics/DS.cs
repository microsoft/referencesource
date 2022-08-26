//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

using System.Diagnostics;
using System.ServiceModel.Channels;

namespace System.ServiceModel.Diagnostics
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Runtime.CompilerServices;

    internal static class DS
    {
        private static readonly DiagnosticSourceBridge s_dsb = new DiagnosticSourceBridge();
        private const string CallThrottleWaitTimestampPropertyName = "System.ServiceModel.Diagnostics.DiagnosticSourceBridge.CallThrottleWaitTimestamp";
        private const string CallThrottleAcquiredTimestampPropertyName = "System.ServiceModel.Diagnostics.DiagnosticSourceBridge.CallThrottleAcquiredTimestamp";
        private const string InstanceThrottleWaitTimestampPropertyName = "System.ServiceModel.Diagnostics.DiagnosticSourceBridge.InstanceThrottleWaitTimestamp";
        private const string InstanceThrottleAcquiredTimestampPropertyName = "System.ServiceModel.Diagnostics.DiagnosticSourceBridge.InstanceThrottleAcquiredTimestamp";

        #region MessageInspectors
        public static bool MessageInspectorIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.MessageInspector);
        }

        public static void DispatchMessageInspectorAfterReceive(Type inspectorType, TimeSpan duration)
        {
            s_dsb.DispatchMessageInspectorAfterReceive(inspectorType.FullName, duration.Ticks);
        }

        public static void DispatchMessageInspectorBeforeSend(Type inspectorType, TimeSpan duration)
        {
            s_dsb.DispatchMessageInspectorBeforeSend(inspectorType.FullName, duration.Ticks);
        }
        public static void ClientMessageInspectorAfterReceive(Type inspectorType, TimeSpan duration)
        {
            s_dsb.ClientMessageInspectorAfterReceive(inspectorType.FullName, duration.Ticks);
        }

        public static void ClientMessageInspectorBeforeSend(Type inspectorType, TimeSpan duration)
        {
            s_dsb.ClientMessageInspectorBeforeSend(inspectorType.FullName, duration.Ticks);
        }
        #endregion // MessageInspectors

        #region IParameterInspector
        public static bool ParameterInspectorIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.ParameterInspector);
        }

        public static void ParameterInspectorAfter(Type inspectorType, TimeSpan duration)
        {
            s_dsb.ParameterInspectorAfter(inspectorType.FullName, duration.Ticks);
        }

        public static void ParameterInspectorBefore(Type inspectorType, TimeSpan duration)
        {
            s_dsb.ParameterInspectorBefore(inspectorType.FullName, duration.Ticks);
        }
        #endregion // IParameterInspector

        #region MessageFormatters
        public static bool MessageFormatterIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.MessageFormatter);
        }

        public static void DispatchMessageFormatterDeserialize(Type formatterType, TimeSpan duration)
        {
            s_dsb.DispatchMessageFormatterDeserialize(formatterType.FullName, duration.Ticks);
        }

        public static void DispatchMessageFormatterSerialize(Type formatterType, TimeSpan duration)
        {
            s_dsb.DispatchMessageFormatterSerialize(formatterType.FullName, duration.Ticks);
        }

        public static void ClientMessageFormatterDeserialize(Type formatterType, TimeSpan duration)
        {
            s_dsb.ClientMessageFormatterDeserialize(formatterType.FullName, duration.Ticks);
        }

        public static void ClientMessageFormatterSerialize(Type formatterType, TimeSpan duration)
        {
            s_dsb.ClientMessageFormatterSerialize(formatterType.FullName, duration.Ticks);
        }
        #endregion // MessageFormatters

        #region OperationSelectors
        public static bool OperationSelectorIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.OperationSelector);
        }

        public static void DispatchSelectOperation(Type selectorType, string selectedOperation, TimeSpan duration)
        {
            s_dsb.DispatchSelectOperation(selectorType.FullName, selectedOperation, duration.Ticks);
        }

        public static void ClientSelectOperation(Type formatterType, string selectedOperation, TimeSpan duration)
        {
            s_dsb.ClientSelectOperation(formatterType.FullName, selectedOperation, duration.Ticks);
        }
        #endregion // OperationSelectors

        #region OperationInvoker
        public static bool OperationInvokerIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.OperationInvoker);
        }

        public static void InvokeOperationStart(Type invokerType, long timestamp)
        {
            s_dsb.InvokeOperationStart(invokerType.FullName, timestamp);
        }

        public static void InvokeOperationStop(long timestamp)
        {
            s_dsb.InvokeOperationStop(timestamp);
        }
        #endregion // OperationInvoker

        #region InstanceProvider
        public static bool InstanceProviderIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.InstanceProvider);
        }

        public static void InstanceProviderGet(Type providerType, object instance, TimeSpan duration)
        {
            s_dsb.InstanceProviderGet(providerType.FullName, RuntimeHelpers.GetHashCode(instance), duration.Ticks);
        }

        public static void InstanceProviderRelease(Type providerType, object instance, TimeSpan duration)
        {
            s_dsb.InstanceProviderRelease(providerType.FullName, RuntimeHelpers.GetHashCode(instance), duration.Ticks);
        }
        #endregion // OperationInvoker

        #region ServiceThrottle
        public static bool ServiceThrottleIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.ServiceThrottle);
        }

        public static void CallThrottleWaiting(Message requestMessage)
        {
            requestMessage.SetProperty(CallThrottleWaitTimestampPropertyName, Stopwatch.GetTimestamp());
        }

        internal static void CallThrottleAcquired(Message requestMessage)
        {
            requestMessage.SetProperty(CallThrottleAcquiredTimestampPropertyName, Stopwatch.GetTimestamp());
        }

        public static void Throttled(Message requestMessage)
        {
            object startTimestampObj, endTimestampObj;
            bool cleanupNeeded = false;
            if (requestMessage.GetProperty(CallThrottleWaitTimestampPropertyName, out startTimestampObj) &&
                requestMessage.GetProperty(CallThrottleAcquiredTimestampPropertyName, out endTimestampObj))
            {
                cleanupNeeded = true;
                long throttleDuration = (long) endTimestampObj - (long) startTimestampObj;
                if (throttleDuration < 0)
                {
                    // When measuring small time periods, Stopwatch.GetTimestamp() can
                    // produce a smaller number on the second call than the first. This is due to 
                    // bugs in the basic input/output system (BIOS) or the hardware
                    // abstraction layer (HAL) on machines with variable-speed CPUs
                    // (e.g. Intel SpeedStep).
                    throttleDuration = 0;
                }

                s_dsb.CallThrottled(throttleDuration);
            }

            if (requestMessage.GetProperty(InstanceThrottleWaitTimestampPropertyName, out startTimestampObj) &&
                requestMessage.GetProperty(InstanceThrottleAcquiredTimestampPropertyName, out endTimestampObj))
            {
                cleanupNeeded = true;
                long throttleDuration = (long)endTimestampObj - (long)startTimestampObj;
                if (throttleDuration < 0)
                {
                    throttleDuration = 0;
                }

                s_dsb.InstanceThrottled(throttleDuration);
            }

            if (cleanupNeeded)
            {
                requestMessage.Properties.Remove(CallThrottleWaitTimestampPropertyName);
                requestMessage.Properties.Remove(CallThrottleAcquiredTimestampPropertyName);
                requestMessage.Properties.Remove(InstanceThrottleWaitTimestampPropertyName);
                requestMessage.Properties.Remove(InstanceThrottleAcquiredTimestampPropertyName);
            }
        }

        public static void InstanceThrottleWaiting(Message requestMessage)
        {
            requestMessage?.SetProperty(InstanceThrottleWaitTimestampPropertyName, Stopwatch.GetTimestamp());
        }

        internal static void InstanceThrottleAcquired(Message requestMessage)
        {
            requestMessage?.SetProperty(InstanceThrottleAcquiredTimestampPropertyName, Stopwatch.GetTimestamp());
        }

        public static void InstanceThrottled(Message requestMessage)
        {
            object startTimestampObj, endTimestampObj;
            if (requestMessage.GetProperty(InstanceThrottleWaitTimestampPropertyName, out startTimestampObj) &&
                requestMessage.GetProperty(InstanceThrottleAcquiredTimestampPropertyName, out endTimestampObj))
            {
                long throttleDuration = (long)endTimestampObj - (long)startTimestampObj;
                if (throttleDuration < 0)
                {
                    // When measuring small time periods, Stopwatch.GetTimestamp() can
                    // produce a smaller number on the second call than the first. This is due to 
                    // bugs in the basic input/output system (BIOS) or the hardware
                    // abstraction layer (HAL) on machines with variable-speed CPUs
                    // (e.g. Intel SpeedStep).
                    throttleDuration = 0;
                }

                s_dsb.InstanceThrottled(throttleDuration);
            }

            // Remove properties outside of if block as there might be only one value in the Properties
            // in which case the if block won't run.
            requestMessage.Properties.Remove(InstanceThrottleWaitTimestampPropertyName);
            requestMessage.Properties.Remove(InstanceThrottleAcquiredTimestampPropertyName);
        }
        #endregion // ServiceThrottle

        #region Authentication
        public static bool AuthenticationIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.Authentication);
        }

        internal static void Authentication(Type authenticationManagerType, bool authenticated, TimeSpan duration)
        {
            s_dsb.Authentication(authenticationManagerType.FullName, authenticated, duration.Ticks);
        }
        #endregion // Authentication

        #region Authorization
        public static bool AuthorizationIsEnabled()
        {
            return s_dsb.IsEnabled(DiagnosticSourceBridge.Keywords.Authorization);
        }

        internal static void Authorization(Type authorizationManagerType, bool authorized, TimeSpan duration)
        {
            s_dsb.Authentication(authorizationManagerType.FullName, authorized, duration.Ticks);
        }
        #endregion // Authorization
    }

    [EventSource(Name = "Microsoft-Windows-Application ServiceModel-DiagnosticSource-Bridge")]
    internal sealed class DiagnosticSourceBridge : EventSource
    {
        // The Keywords attribute property must not be set last as there's a quirk in the FxAnnotate build tool which corrupts
        // the attribute if the last property is an Enum which isn't dervied from Int32.
        #region MessageInspectors
        [Event(EventIds.DispatchMessageInspectorAfterReceive, Keywords = Keywords.MessageInspector, Level = EventLevel.Verbose)]
        public void DispatchMessageInspectorAfterReceive(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.DispatchMessageInspectorAfterReceive, TypeName, Duration);
        }

        [Event(EventIds.DispatchMessageInspectorBeforeSend, Keywords = Keywords.MessageInspector, Level = EventLevel.Verbose)]
        public void DispatchMessageInspectorBeforeSend(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.DispatchMessageInspectorBeforeSend, TypeName, Duration);
        }

        [Event(EventIds.ClientMessageInspectorAfterReceive, Keywords = Keywords.MessageInspector, Level = EventLevel.Verbose)]
        public void ClientMessageInspectorAfterReceive(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ClientMessageInspectorAfterReceive, TypeName, Duration);
        }

        [Event(EventIds.ClientMessageInspectorBeforeSend, Keywords = Keywords.MessageInspector, Level = EventLevel.Verbose)]
        public void ClientMessageInspectorBeforeSend(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ClientMessageInspectorBeforeSend, TypeName, Duration);
        }
        #endregion // MessageInspectors

        #region IParameterInspector
        [Event(EventIds.ParameterInspectorAfter, Keywords = Keywords.ParameterInspector, Level = EventLevel.Verbose)]
        public void ParameterInspectorAfter(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ParameterInspectorAfter, TypeName, Duration);
        }

        [Event(EventIds.ParameterInspectorBefore, Keywords = Keywords.ParameterInspector, Level = EventLevel.Verbose)]
        public void ParameterInspectorBefore(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ParameterInspectorBefore, TypeName, Duration);
        }
        #endregion // IParameterInspector

        #region MessageFormatters
        [Event(EventIds.DispatchMessageFormatterDeserialize, Keywords = Keywords.MessageFormatter, Level = EventLevel.Verbose)]
        public void DispatchMessageFormatterDeserialize(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.DispatchMessageFormatterDeserialize, TypeName, Duration);
        }

        [Event(EventIds.DispatchMessageFormatterSerialize, Keywords = Keywords.MessageFormatter, Level = EventLevel.Verbose)]
        public void DispatchMessageFormatterSerialize(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.DispatchMessageFormatterSerialize, TypeName, Duration);
        }

        [Event(EventIds.ClientMessageFormatterDeserialize, Keywords = Keywords.MessageFormatter, Level = EventLevel.Verbose)]
        public void ClientMessageFormatterDeserialize(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ClientMessageFormatterDeserialize, TypeName, Duration);
        }

        [Event(EventIds.ClientMessageFormatterSerialize, Keywords = Keywords.MessageFormatter, Level = EventLevel.Verbose)]
        public void ClientMessageFormatterSerialize(string TypeName, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ClientMessageFormatterSerialize, TypeName, Duration);
        }
        #endregion // MessageFormatters

        #region OperationSelectors
        [Event(EventIds.DispatchSelectOperation, Keywords = Keywords.OperationSelector, Level = EventLevel.Verbose)]
        public void DispatchSelectOperation(string TypeName, string SelectedOperation, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.DispatchSelectOperation, TypeName, SelectedOperation, Duration);
        }

        [Event(EventIds.ClientSelectOperation, Keywords = Keywords.OperationSelector, Level = EventLevel.Verbose)]
        public void ClientSelectOperation(string TypeName, string SelectedOperation, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.ClientSelectOperation, TypeName, SelectedOperation, Duration);
        }
        #endregion // OperationSelectors

        #region OperationInvoker
        [Event(EventIds.InvokeOperationStart, Keywords = Keywords.OperationInvoker, Level = EventLevel.Verbose)]
        public void InvokeOperationStart(string TypeName, long Timestamp)
        {
            WriteEvent(EventIds.InvokeOperationStart, TypeName, Timestamp);
        }

        [Event(EventIds.InvokeOperationStop, Keywords = Keywords.OperationInvoker, Level = EventLevel.Verbose)]
        public void InvokeOperationStop(long Timestamp)
        {
            WriteEvent(EventIds.InvokeOperationStop, Timestamp);
        }
        #endregion // OperationInvoker

        #region InstanceProvider
        [Event(EventIds.InstanceProviderGet, Keywords = Keywords.InstanceProvider, Level = EventLevel.Verbose)]
        public void InstanceProviderGet(string TypeName, int InstanceHash, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.InstanceProviderGet, TypeName, InstanceHash, Duration);
        }

        [Event(EventIds.InstanceProviderRelease, Keywords = Keywords.InstanceProvider, Level = EventLevel.Verbose)]
        public void InstanceProviderRelease(string TypeName, int InstanceHash, long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.InstanceProviderRelease, TypeName, InstanceHash, Duration);
        }
        #endregion // InstanceProvider

        #region ServiceThrottle
        [Event(EventIds.CallThrottled, Keywords = Keywords.ServiceThrottle, Level = EventLevel.Verbose)]
        public void CallThrottled(long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.CallThrottled, Duration);
        }

        [Event(EventIds.InstanceThrottled, Keywords = Keywords.ServiceThrottle, Level = EventLevel.Verbose)]
        public void InstanceThrottled(long Duration /* TimeSpan.Ticks*/)
        {
            WriteEvent(EventIds.InstanceThrottled, Duration);
        }
        #endregion // SeviceThrottle

        #region Authentication
        [Event(EventIds.Authentication, Keywords = Keywords.Authentication, Level = EventLevel.Verbose)]
        public void Authentication(string TypeName, bool Authenticated, long Duration)
        {
            WriteEvent(EventIds.Authentication, TypeName, Authenticated, Duration);
        }
        #endregion // Authentication

        #region Authorization
        [Event(EventIds.Authorization, Keywords = Keywords.Authorization, Level = EventLevel.Verbose)]
        public void Authorization(string TypeName, bool Authorized, long Duration)
        {
            WriteEvent(EventIds.Authorization, TypeName, Authorized, Duration);
        }
        #endregion // Authentication

        internal bool IsEnabled(EventKeywords keywords)
        {
            return this.IsEnabled() && this.IsEnabled(EventLevel.Verbose, keywords);
        }

        public class EventIds
        {
            public const int DispatchMessageInspectorAfterReceive = 1;
            public const int DispatchMessageInspectorBeforeSend = 2;
            public const int ClientMessageInspectorAfterReceive = 3;
            public const int ClientMessageInspectorBeforeSend = 4;
            public const int ParameterInspectorAfter = 5;
            public const int ParameterInspectorBefore = 6;
            public const int DispatchMessageFormatterDeserialize = 7;
            public const int DispatchMessageFormatterSerialize = 8;
            public const int ClientMessageFormatterDeserialize = 9;
            public const int ClientMessageFormatterSerialize = 10;
            public const int DispatchSelectOperation = 11;
            public const int ClientSelectOperation = 12;
            public const int InvokeOperationStart = 13;
            public const int InvokeOperationStop = 14;
            public const int InstanceProviderGet = 15;
            public const int InstanceProviderRelease = 16;
            public const int CallThrottled = 17;
            public const int InstanceThrottled = 18;
            public const int Authentication = 19;
            public const int Authorization = 20;
        }

        public class Keywords
        {
            public const EventKeywords MessageInspector = (EventKeywords) 0x01L;
            public const EventKeywords ParameterInspector = (EventKeywords) 0x02L;
            public const EventKeywords MessageFormatter = (EventKeywords) 0x04L;
            public const EventKeywords OperationSelector = (EventKeywords) 0x08L;
            public const EventKeywords OperationInvoker = (EventKeywords) 0x10L;
            public const EventKeywords InstanceProvider = (EventKeywords) 0x20L;
            public const EventKeywords ServiceThrottle = (EventKeywords) 0x40L;
            public const EventKeywords Authentication = (EventKeywords) 0x80L;
            public const EventKeywords Authorization = (EventKeywords) 0x100L;
        }
    }
}
