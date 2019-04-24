using System.ComponentModel;
using System.Diagnostics.Contracts;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading;

namespace System.Net.Http
{
    // This class implements all the necessary COM interfaces for working with the Win8 RTC NotificationChannel.
    // The CLR handles IInspectable on our behalf.
    [ComVisible(true)]
    internal class RtcRequestMessage : HttpRequestMessage, INetworkTransportSettings, INotificationTransportSync
    {
        // This is the RTC GUID for ApplySetting and QuerySetting
        private static readonly Guid TransportSettingsId = new Guid("6B59819A-5CAE-492D-A901-2A3C2C50164F");
        internal RtcState state;

        internal RtcRequestMessage(HttpMethod method, Uri uri)
            : base(method, uri)
        {
            state = new RtcState();
        }

        #region INetworkTransportSettings

        [SecuritySafeCritical]
        public void ApplySetting([In] ref TRANSPORT_SETTING_ID settingId, [In] int lengthIn, [In] IntPtr valueIn,
            out int lengthOut, out IntPtr valueOut)
        {
            if (!TransportSettingsId.Equals(settingId.Guid))
            {
                // This method is called from WinRT. The CLR will map the exception type to an HResult
                // and discard any error message.
                throw new NotSupportedException();
            }
            if (valueIn == IntPtr.Zero)
            {
                throw new ArgumentNullException("valueIn");
            }

            // SettingId GUID + valueIn (also a GUID)
            byte[] settingsGuid = settingId.Guid.ToByteArray();
            byte[] buffer = new byte[settingsGuid.Length + lengthIn];
            settingsGuid.CopyTo(buffer, 0);
            if (lengthIn > 0)
            {
                // Store it for use later during the request
                Marshal.Copy(valueIn, buffer, settingsGuid.Length, lengthIn);
            }
            state.inputData = buffer;

            lengthOut = 0;
            valueOut = IntPtr.Zero;
        }

        [SecuritySafeCritical]
        public void QuerySetting([In] ref TRANSPORT_SETTING_ID settingId, [In] int lengthIn, [In] IntPtr valueIn, 
            out int lengthOut, out IntPtr valueOut)
        {
            if (!TransportSettingsId.Equals(settingId.Guid))
            {
                // This method is called from WinRT. The CLR will map the exception type to an HResult
                // and discard any error message.
                throw new NotSupportedException();
            }
            // Note that we ignore lengthIn and valueIn for this operation.  
            // It should be the same as that from ApplySetting so we'll just re-use those values.

            // Wait until we've connected and set the socket option.  If it fails then the request will be aborted.  
            // If it suceeds then we can assume that the socket is now push-enabled.
            state.connectComplete.WaitOne();

            byte[] result;
            if (state.result != 0)
            {
                // This is how we report errors to NotificationChannel.WaitForPushEnabled().
                // The CLR will unwrap the exception and return the error code.
                throw new Win32Exception(state.result);
            }
            else if (state.outputData != null)
            {
                result = state.outputData;
            }
            else
            {
                result = BitConverter.GetBytes((int)RtcState.ControlChannelTriggerStatus.TransportDisconnected);
            }

            lengthOut = result.Length;
            // COM memory marshal. It's the caller's responsibility to free the memory.
            valueOut = Marshal.AllocCoTaskMem(lengthOut);
            Marshal.Copy(result, 0, valueOut, lengthOut);
        }

        #endregion INetworkTransportSettings

        #region INotificationTransportSync

        public void CompleteDelivery()
        {
            // No synchronization required.  The developer just needs to do Task.Wait() or Task.Result to wait for 
            // the request to finish.
        }

        public void Flush()
        {
            state.flushComplete.WaitOne();
        }

        #endregion INotificationTransportSync
    }

    // These extensions exist so that HttpClientHandler doesn't have to check for the special RtcRequestMessage Type.
    internal static class RtcRequestMessageExtensions
    {
        internal static void SetRtcOptions(this HttpRequestMessage request, HttpWebRequest webRequest)
        {
#if !NET_4
            RtcRequestMessage rtcRequest = request as RtcRequestMessage;
            if (rtcRequest != null)
            {
                // Data to pass into the socket IOControl
                webRequest.RtcState = rtcRequest.state;
                
                // Enforce Rtc stability requirements.  We can't have the server responding at all
                // until the final response.  This may require a primer request be sent.
                webRequest.ServicePoint.Expect100Continue = false;
                webRequest.PreAuthenticate = true;
                webRequest.KeepAlive = false;
                webRequest.AllowAutoRedirect = false;
                webRequest.Pipelined = false;
            }
#endif
        }

        internal static void MarkRtcFlushComplete(this HttpRequestMessage request)
        {
            RtcRequestMessage rtcRequest = request as RtcRequestMessage;
            if (rtcRequest != null)
            {
                // We've sent all of the request headers and body (if any).
                rtcRequest.state.flushComplete.Set();
            }
        }

        internal static void AbortRtcRequest(this HttpRequestMessage request)
        {
            RtcRequestMessage rtcRequest = request as RtcRequestMessage;
            if (rtcRequest != null)
            {
                rtcRequest.state.Abort();
            }
        }
    }
}