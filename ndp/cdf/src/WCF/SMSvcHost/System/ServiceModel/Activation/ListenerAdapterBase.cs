//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Runtime.Versioning;
    using System.Security.Principal;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;    
    using System.ServiceModel.Diagnostics;    
    using System.Threading;    

    abstract class ListenerAdapterBase
    {
        WebHostUnsafeNativeMethods.WebhostListenerCallbacks listenerCallbacks;
        int protocolHandle = 0;
        int closed;

        const int WebhostMajorVersion = 7;
        const int WebhostMinorVersion = 0;
        static WebHostUnsafeNativeMethods.SafeFreeLibrary webHostIpm;
        static WebHostUnsafeNativeMethods.WebhostGetVersion webhostGetVersion;
        static WebHostUnsafeNativeMethods.WebhostRegisterProtocol webhostRegisterProtocol;
        static WebHostUnsafeNativeMethods.WebhostOpenListenerChannelInstance webhostOpenListenerChannelInstance;
        static WebHostUnsafeNativeMethods.WebhostCloseAllListenerChannelInstances webhostCloseAllListenerChannelInstances;
        static WebHostUnsafeNativeMethods.WebhostUnregisterProtocol webhostUnregisterProtocol;
        EventTraceActivity eventTraceActivity;

        [ResourceConsumption(ResourceScope.Process)]
        static ListenerAdapterBase()
        {
#pragma warning suppress 56523 // Microsoft, the Win32Exception ctor calls Marshal.GetLastWin32Error
            webHostIpm = WebHostUnsafeNativeMethods.LoadLibraryEx(SMSvcHost.ListenerAdapterNativeLibrary, IntPtr.Zero, WebHostUnsafeNativeMethods.LOAD_WITH_ALTERED_SEARCH_PATH);
            if (webHostIpm.IsInvalid)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new Win32Exception());
            }

            webhostGetVersion = (WebHostUnsafeNativeMethods.WebhostGetVersion)GetProcDelegate<WebHostUnsafeNativeMethods.WebhostGetVersion>(webHostIpm, "WebhostGetVersion");
            webhostRegisterProtocol = (WebHostUnsafeNativeMethods.WebhostRegisterProtocol)GetProcDelegate<WebHostUnsafeNativeMethods.WebhostRegisterProtocol>(webHostIpm, "WebhostRegisterProtocol");
            webhostUnregisterProtocol = (WebHostUnsafeNativeMethods.WebhostUnregisterProtocol)GetProcDelegate<WebHostUnsafeNativeMethods.WebhostUnregisterProtocol>(webHostIpm, "WebhostUnregisterProtocol");
            webhostOpenListenerChannelInstance = (WebHostUnsafeNativeMethods.WebhostOpenListenerChannelInstance)GetProcDelegate<WebHostUnsafeNativeMethods.WebhostOpenListenerChannelInstance>(webHostIpm, "WebhostOpenListenerChannelInstance");
            webhostCloseAllListenerChannelInstances = (WebHostUnsafeNativeMethods.WebhostCloseAllListenerChannelInstances)GetProcDelegate<WebHostUnsafeNativeMethods.WebhostCloseAllListenerChannelInstances>(webHostIpm,
                "WebhostCloseAllListenerChannelInstances");
        }

        protected ListenerAdapterBase(string protocolName)
        {
            if (string.IsNullOrEmpty(protocolName))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("protocolName"));
            }
            this.ProtocolName = protocolName;
            listenerCallbacks = new WebHostUnsafeNativeMethods.WebhostListenerCallbacks();
            listenerCallbacks.dwBytesInCallbackStructure = Marshal.SizeOf(listenerCallbacks);
            listenerCallbacks.webhostListenerConfigManagerConnected = new WebHostUnsafeNativeMethods.WebhostListenerConfigManagerConnected(onConfigManagerConnected);
            listenerCallbacks.webhostListenerConfigManagerDisconnected = new WebHostUnsafeNativeMethods.WebhostListenerConfigManagerDisconnected(onConfigManagerDisconnected);
            listenerCallbacks.webhostListenerConfigManagerInitializationCompleted = new WebHostUnsafeNativeMethods.WebhostListenerConfigManagerInitializationCompleted(onConfigManagerInitializationCompleted);
            listenerCallbacks.webhostListenerApplicationPoolCreated = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolCreated(onApplicationPoolCreated);
            listenerCallbacks.webhostListenerApplicationPoolDeleted = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolDeleted(onApplicationPoolDeleted);
            listenerCallbacks.webhostListenerApplicationPoolIdentityChanged = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolIdentityChanged(onApplicationPoolIdentityChanged);
            listenerCallbacks.webhostListenerApplicationPoolStateChanged = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolStateChanged(onApplicationPoolStateChanged);
            listenerCallbacks.webhostListenerApplicationPoolCanOpenNewListenerChannelInstance = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolCanOpenNewListenerChannelInstance(onApplicationPoolCanLaunchQueueInstance);
            listenerCallbacks.webhostListenerApplicationPoolAllListenerChannelInstancesStopped = new WebHostUnsafeNativeMethods.WebhostListenerApplicationPoolAllListenerChannelInstancesStopped(onApplicationPoolAllQueueInstancesStopped);
            listenerCallbacks.webhostListenerApplicationCreated = new WebHostUnsafeNativeMethods.WebhostListenerApplicationCreated(onApplicationCreated);
            listenerCallbacks.webhostListenerApplicationDeleted = new WebHostUnsafeNativeMethods.WebhostListenerApplicationDeleted(onApplicationDeleted);
            listenerCallbacks.webhostListenerApplicationBindingsChanged = new WebHostUnsafeNativeMethods.WebhostListenerApplicationBindingsChanged(onApplicationBindingsChanged);
            listenerCallbacks.webhostListenerApplicationAppPoolChanged = new WebHostUnsafeNativeMethods.WebhostListenerApplicationAppPoolChanged(onApplicationAppPoolChanged);
            listenerCallbacks.webhostListenerApplicationRequestsBlockedChanged = new WebHostUnsafeNativeMethods.WebhostListenerApplicationRequestsBlockedChanged(onApplicationRequestsBlockedChanged);
        }

        protected string ProtocolName 
        {
            get;
            private set;
        }

        protected EventTraceActivity EventTraceActivity
        {
            get
            {
                if (this.eventTraceActivity == null)
                {
                    this.eventTraceActivity = new EventTraceActivity();
                }
                return this.eventTraceActivity;
            }
        }


        static Delegate GetProcDelegate<TDelegate>(WebHostUnsafeNativeMethods.SafeFreeLibrary library, string procName)
        {
#pragma warning suppress 56523 // Microsoft, the Win32Exception ctor calls Marshal.GetLastWin32Error
            IntPtr funcPtr = WebHostUnsafeNativeMethods.GetProcAddress(library, procName);
            if (funcPtr == IntPtr.Zero)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new Win32Exception(SR.GetString(SR.WebHostProcNotFound,
                    procName, SMSvcHost.ListenerAdapterNativeLibrary)));
            }

            return Marshal.GetDelegateForFunctionPointer(funcPtr, typeof(TDelegate));
        }

        protected string[] ParseBindings(IntPtr bindingsMultiSz, int numberOfBindings)
        {
            if (bindingsMultiSz == IntPtr.Zero)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("bindingsMultiSz"));
            }
            if (numberOfBindings < 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException("numberOfBindings"));
            }
            string[] bindings = new string[numberOfBindings];
            unsafe
            {
                ushort* bindingsBufferPtr = (ushort*)bindingsMultiSz;
                for (int i = 0; i < numberOfBindings; i++)
                {
                    string bindingString = Marshal.PtrToStringUni((IntPtr)bindingsBufferPtr);
                    if (string.IsNullOrEmpty(bindingString))
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException("bindingsMultiSz"));
                    }
                    bindings[i] = bindingString;
                    bindingsBufferPtr += bindingString.Length + 1;
                }
            }
            return bindings;
        }

        void onApplicationAppPoolChanged(IntPtr context, string appKey, string appPoolId)
        {
            this.OnApplicationAppPoolChanged(appKey, appPoolId);
        }
        protected abstract void OnApplicationAppPoolChanged(string appKey, string appPoolId);

        void onApplicationBindingsChanged(IntPtr context, string appKey, IntPtr bindingsMultiSz, int numberOfBindings)
        {
            this.OnApplicationBindingsChanged(appKey, bindingsMultiSz, numberOfBindings);
        }
        protected abstract void OnApplicationBindingsChanged(string appKey, IntPtr bindingsMultiSz, int numberOfBindings);

        void onApplicationCreated(IntPtr context, string appKey, string path, int siteId, string appPoolId, IntPtr bindingsMultiSz, int numberOfBindings, bool requestsBlocked)
        {
            this.OnApplicationCreated(appKey, path, siteId, appPoolId, bindingsMultiSz, numberOfBindings, requestsBlocked);
        }
        protected abstract void OnApplicationCreated(string appKey, string path, int siteId, string appPoolId, IntPtr bindingsMultiSz, int numberOfBindings, bool requestsBlocked);

        void onApplicationDeleted(IntPtr context, string appKey)
        {
            this.OnApplicationDeleted(appKey);
        }
        protected abstract void OnApplicationDeleted(string appKey);

        void onApplicationPoolAllQueueInstancesStopped(IntPtr context, string appPoolId, int listenerChannelId)
        {
            this.OnApplicationPoolAllQueueInstancesStopped(appPoolId, listenerChannelId);
        }
        protected abstract void OnApplicationPoolAllQueueInstancesStopped(string appPoolId, int listenerChannelId);

        void onApplicationPoolCanLaunchQueueInstance(IntPtr context, string appPoolId, int listenerChannelId)
        {
            this.OnApplicationPoolCanLaunchQueueInstance(appPoolId, listenerChannelId);
        }
        protected abstract void OnApplicationPoolCanLaunchQueueInstance(string appPoolId, int listenerChannelId);

        void onApplicationPoolCreated(IntPtr context, string appPoolId, IntPtr sid)
        {
            this.OnApplicationPoolCreated(appPoolId, new SecurityIdentifier(sid));
        }
        protected abstract void OnApplicationPoolCreated(string appPoolId, SecurityIdentifier sid);

        void onApplicationPoolDeleted(IntPtr context, string appPoolId)
        {
            this.OnApplicationPoolDeleted(appPoolId);
        }
        protected abstract void OnApplicationPoolDeleted(string appPoolId);

        void onApplicationPoolIdentityChanged(IntPtr context, string appPoolId, IntPtr sid)
        {
            this.OnApplicationPoolIdentityChanged(appPoolId, new SecurityIdentifier(sid));
        }
        protected abstract void OnApplicationPoolIdentityChanged(string appPoolId, SecurityIdentifier sid);

        void onApplicationPoolStateChanged(IntPtr context, string appPoolId, bool isEnabled)
        {
            this.OnApplicationPoolStateChanged(appPoolId, isEnabled);
        }
        protected abstract void OnApplicationPoolStateChanged(string appPoolId, bool isEnabled);

        void onApplicationRequestsBlockedChanged(IntPtr context, string appKey, bool requestsBlocked)
        {
            this.OnApplicationRequestsBlockedChanged(appKey, requestsBlocked);
        }
        protected abstract void OnApplicationRequestsBlockedChanged(string appKey, bool requestsBlocked);

        void onConfigManagerConnected(IntPtr context)
        {
            this.OnConfigManagerConnected();
        }

        protected abstract void OnConfigManagerConnected();

        void onConfigManagerDisconnected(IntPtr context, int hresult)
        {
            this.OnConfigManagerDisconnected(hresult);
        }
        protected abstract void OnConfigManagerDisconnected(int hresult);

        void onConfigManagerInitializationCompleted(IntPtr context)
        {
            this.OnConfigManagerInitializationCompleted();
        }
        protected abstract void OnConfigManagerInitializationCompleted();

        protected void Close()
        {
            if (Interlocked.Increment(ref closed) > 1)
            {
                return;
            }
            int hresult = webhostUnregisterProtocol(protocolHandle);
            if (hresult != 0)
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.WasWebHostAPIFailed, SR.GetString(SR.TraceCodeWasWebHostAPIFailed), 
                        new StringTraceRecord("HRESULT", SR.GetString(SR.TraceCodeWasWebHostAPIFailed, "WebhostUnregisterProtocol",
                        hresult.ToString(CultureInfo.CurrentCulture))), this, null);
                }
            }
            if (TD.WebhostUnregisterProtocolFailedIsEnabled())
            {
                //TraceCodeWasWebHostAPIFailed
                TD.WebhostUnregisterProtocolFailed(this.EventTraceActivity, hresult.ToString(CultureInfo.CurrentCulture));
            }
        }

        protected int OpenListenerChannelInstance(string appPoolId, int listenerChannelId, byte[] queueBlob)
        {
            Fx.Assert(appPoolId != null, "");

            int queueBlobLength = (queueBlob != null) ? queueBlob.Length : 0;
            return webhostOpenListenerChannelInstance(protocolHandle, appPoolId, listenerChannelId, queueBlob, queueBlobLength);
        }

        internal virtual void Open()
        {
            int major, minor;
            int hresult = webhostGetVersion(out major, out minor);
            if (hresult != 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new Win32Exception(hresult));
            }

            if (major != WebhostMajorVersion ||
                minor != WebhostMinorVersion)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new PlatformNotSupportedException(
                    SR.GetString(SR.WebHostVersionMismatch, WebhostMajorVersion, WebhostMinorVersion,
                    major, minor)));
            }

            hresult = webhostRegisterProtocol(ProtocolName, ref listenerCallbacks, IntPtr.Zero, out protocolHandle);
            if (hresult != 0 || protocolHandle == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new Win32Exception(hresult));
            }
        }

        protected int CloseAllListenerChannelInstances(string appPoolId, int listenerChannelId)
        {
            if (string.IsNullOrEmpty(appPoolId))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentNullException("appPoolId"));
            }
            return webhostCloseAllListenerChannelInstances(protocolHandle, appPoolId, listenerChannelId);
        }
    }
}

