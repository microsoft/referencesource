
namespace System.Net.Http
{
    // This class hides the internal details of the RtcRequestMessage.
    // It can only be used in Windows Store apps.
    public static class RtcRequestFactory
    {
        public static HttpRequestMessage Create(HttpMethod method, Uri uri)
        {
            if (!ComNetOS.IsWin8orLater || !Microsoft.Win32.UnsafeNativeMethods.IsPackagedProcess.Value)
            {
                throw new PlatformNotSupportedException(WrSR.net_rtc_windowsstoreapp_required);
            }

            return new RtcRequestMessage(method, uri);
        }
    }
}