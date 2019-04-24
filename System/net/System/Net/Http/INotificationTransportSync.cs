using System.Runtime.InteropServices;

namespace System.Net.Http
{
    // For integration with the Win8 Real Time Communications broker. See RtcRequestMessage.
    [ComImport]
    [ComVisible(true)]
    [Guid("79eb1402-0ab8-49c0-9e14-a1ae4ba93058")] 
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface INotificationTransportSync
    {
        void CompleteDelivery();
        void Flush();
    }
}