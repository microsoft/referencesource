//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Runtime.ConstrainedExecution;
    using System.Runtime.Versioning;
    using Microsoft.Win32.SafeHandles;

    [SuppressUnmanagedCodeSecurityAttribute()]
    // functions in %windir%\system32\inetsrv\wbhstipm.dll
    // headers in //depot/devdiv/private/webnetorcas/ndp/iis/iisrearc/core/inc/wbhst_entry.h
    // implementation in //depot/devdiv/private/webnetorcas/ndp/iis/iisrearc/core/ap/wbhstipm/dll/clientprotocol.cxx
    static class WebHostUnsafeNativeMethods
    {
        const string KERNEL32 = "kernel32.dll";

        internal const int LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008;
        internal delegate int WebhostGetVersion(out int major, out int minor);
        internal delegate void WebhostListenerApplicationAppPoolChanged(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appKey, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId);
        internal delegate void WebhostListenerApplicationBindingsChanged(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appKey, IntPtr bindingsMultiSz, int numberOfBindings);
        internal delegate void WebhostListenerApplicationCreated(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appKey, [MarshalAs(UnmanagedType.LPWStr)] string path, int siteId, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, IntPtr bindingsMultiSz, int numberOfBindings, bool requestsBlocked);
        internal delegate void WebhostListenerApplicationDeleted(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appKey);
        internal delegate void WebhostListenerApplicationPoolAllListenerChannelInstancesStopped(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, int listenerChannelId);
        internal delegate void WebhostListenerApplicationPoolCanOpenNewListenerChannelInstance(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, int listenerChannelId);
        internal delegate void WebhostListenerApplicationPoolCreated(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, IntPtr sid);
        internal delegate void WebhostListenerApplicationPoolDeleted(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId);
        internal delegate void WebhostListenerApplicationPoolIdentityChanged(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, IntPtr sid);
        internal delegate void WebhostListenerApplicationPoolStateChanged(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, bool isEnabled);
        internal delegate void WebhostListenerApplicationRequestsBlockedChanged(IntPtr context, [MarshalAs(UnmanagedType.LPWStr)] string appKey, bool requestsBlocked);

        internal delegate void WebhostListenerConfigManagerConnected(IntPtr context);
        internal delegate void WebhostListenerConfigManagerConnectRejected(IntPtr context, int hresult);
        internal delegate void WebhostListenerConfigManagerDisconnected(IntPtr context, int hresult);
        internal delegate void WebhostListenerConfigManagerInitializationCompleted(IntPtr context);
        internal delegate int WebhostRegisterProtocol([MarshalAs(UnmanagedType.LPWStr)] string protocolId, ref WebhostListenerCallbacks listenerCallbacks, IntPtr context, out int protocolHandle);
        internal delegate int WebhostOpenListenerChannelInstance(int protocolHandle, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, int listenerChannelId, byte[] queueBlob, int queueBlobByteCount);
        internal delegate int WebhostCloseAllListenerChannelInstances(int protocolHandle, [MarshalAs(UnmanagedType.LPWStr)] string appPoolId, int listenerChannelId);
        internal delegate int WebhostUnregisterProtocol(int protocolHandle);

        [DllImport(KERNEL32, CharSet = CharSet.Ansi, SetLastError = true, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        [ResourceExposure(ResourceScope.None)]
        internal static extern IntPtr GetProcAddress(
            [In] SafeFreeLibrary hModule,
            [In] string lpProcName
            );

        [DllImport(KERNEL32, CharSet = CharSet.Unicode, SetLastError = true)]
        [ResourceExposure(ResourceScope.Process)]
        internal static extern SafeFreeLibrary LoadLibraryEx(
            [In] string lpFileName,
            [In] IntPtr hFile,
            [In] int dwFlags
            );

        [StructLayout(LayoutKind.Sequential)]
        internal struct WebhostListenerCallbacks
        {
            internal int dwBytesInCallbackStructure;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerConfigManagerConnected webhostListenerConfigManagerConnected;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerConfigManagerDisconnected webhostListenerConfigManagerDisconnected;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerConfigManagerInitializationCompleted webhostListenerConfigManagerInitializationCompleted;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolCreated webhostListenerApplicationPoolCreated;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolDeleted webhostListenerApplicationPoolDeleted;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolIdentityChanged webhostListenerApplicationPoolIdentityChanged;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolStateChanged webhostListenerApplicationPoolStateChanged;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolCanOpenNewListenerChannelInstance webhostListenerApplicationPoolCanOpenNewListenerChannelInstance;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationPoolAllListenerChannelInstancesStopped webhostListenerApplicationPoolAllListenerChannelInstancesStopped;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationCreated webhostListenerApplicationCreated;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationDeleted webhostListenerApplicationDeleted;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationBindingsChanged webhostListenerApplicationBindingsChanged;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationAppPoolChanged webhostListenerApplicationAppPoolChanged;
            [MarshalAs(UnmanagedType.FunctionPtr)]
            internal WebhostListenerApplicationRequestsBlockedChanged webhostListenerApplicationRequestsBlockedChanged;
        }

        internal sealed class SafeFreeLibrary : SafeHandleZeroOrMinusOneIsInvalid
        {
            const string KERNEL32 = "kernel32.dll";

            internal SafeFreeLibrary()
                : base(true)
            {
            }

            [DllImport(KERNEL32, SetLastError = true), SuppressUnmanagedCodeSecurity]
            [ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
            [ResourceExposure(ResourceScope.None)]
            private static extern bool FreeLibrary(IntPtr hModule);

            protected override bool ReleaseHandle()
            {
                return FreeLibrary(handle);
            }
        }
    }
}

