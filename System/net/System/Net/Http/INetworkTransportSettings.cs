using System.Runtime.InteropServices;

namespace System.Net.Http
{
    // For integration with the Win8 Real Time Communications broker. See RtcRequestMessage.
    [ComImport]
    [ComVisible(true)]
    [Guid("5e7abb2c-f2c1-4a61-bd35-deb7a08ab0f1")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface INetworkTransportSettings
    {
        void ApplySetting([In] ref TRANSPORT_SETTING_ID settingId, [In] int lengthIn, [In] IntPtr valueIn, 
            out int lengthOut, out IntPtr valueOut);
        
        void QuerySetting([In] ref TRANSPORT_SETTING_ID settingId, [In] int lengthIn, [In] IntPtr valueIn,
            out int lengthOut, out IntPtr valueOut);
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    internal struct TRANSPORT_SETTING_ID
    {
        public Guid Guid;
    }
}