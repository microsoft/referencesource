// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Internal types/classes used by Location API
**
** Purpose: Represents helpers and Location API COM interops used by the API
**
=============================================================================*/

using System;
using System.Security;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace System.Device.Location.Internal
{
    #region Helpers
    internal static class Utility
    {
        [Conditional("DEBUG")]
        public static void DebugAssert(bool condition, string message)
        {
            System.Diagnostics.Debug.Assert(condition, message);
        }

        [Conditional("DEBUG")]
        public static void Trace(string message)
        {
#if !SILVERLIGHT
            System.Diagnostics.Trace.WriteLine(message);
#endif
        }
    }
    #endregion

#if SILVERLIGHT
    internal enum VarEnum
    {
        VT_EMPTY  = 0,
        VT_R8     = 5,
        VT_LPWSTR = 8
    }
#endif

    #region PROPVARIANT
    //
    // The equivalent of PROPVARIANT struct defined in propidl.h
    // Only double and LPWSTR are used in Location platform
    //
    [StructLayout(LayoutKind.Explicit, Size=24)]
    internal class PROPVARIANT : IDisposable
    {
        [FieldOffset(0)] public VarEnum vt = VarEnum.VT_EMPTY;
        [FieldOffset(8)] private Double dblVal;
        [FieldOffset(8)] private IntPtr other;

        [DllImport("Ole32")]
        private static extern void PropVariantClear(PROPVARIANT pvar);

        #region IDisposable
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~PROPVARIANT()
        {
            Dispose(false);
        }

        private void Dispose(Boolean disposing)
        {
            Utility.Trace("PROPVARIANT.Dispose(disposing=" + disposing.ToString() + ")");
            Clear();
        }
        #endregion

        [SecuritySafeCritical]
        public void Clear()
        {
            if (vt != VarEnum.VT_EMPTY)
            {
                PropVariantClear(this);
                vt = VarEnum.VT_EMPTY;
            }
        }

        [SecuritySafeCritical]
        public Object GetValue()
        {
            switch (vt)
            {
                case VarEnum.VT_R8: return dblVal;
                case VarEnum.VT_LPWSTR: return Marshal.PtrToStringUni(other);
                default: return null;
            }
        }
    }
    #endregion

    #region PROPERTYKEY
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    internal struct PROPERTYKEY
    {
        public PROPERTYKEY(Int32 propertyId)
        {
            pid = propertyId;
            fmtid = new Guid("055C74D8-CA6F-47D6-95C6-1ED3637A0FF4");
        }
        public readonly Guid fmtid;
        public readonly Int32 pid;
    }
    #endregion

    #region Keys
    //
    // These are the keys not available on ILatLongReport interface
    //
    internal struct LocationPropertyKey
    {
        public static readonly PROPERTYKEY Speed         = new PROPERTYKEY(6);
        public static readonly PROPERTYKEY Heading       = new PROPERTYKEY(7);
        public static readonly PROPERTYKEY AddressLine1  = new PROPERTYKEY(23);
        public static readonly PROPERTYKEY AddressLine2  = new PROPERTYKEY(24);
        public static readonly PROPERTYKEY City          = new PROPERTYKEY(25);
        public static readonly PROPERTYKEY StateProvince = new PROPERTYKEY(26);
        public static readonly PROPERTYKEY PostalCode    = new PROPERTYKEY(27);
        public static readonly PROPERTYKEY CountryRegion = new PROPERTYKEY(28);

    }

    internal struct LocationReportKey
    {
        internal static readonly Guid LatLongReport = new Guid("7FED806D-0EF8-4f07-80AC-36A0BEAE3134");
        internal static readonly Guid CivicAddressReport = new Guid("C0B19F70-4ADF-445d-87F2-CAD8FD711792");
    }
    #endregion

    #region SYSTEMTIME
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    internal struct SYSTEMTIME
    {
        public ushort wYear;
        public ushort wMonth;
        public ushort wDayOfWeek;
        public ushort wDay;
        public ushort wHour;
        public ushort wMinute;
        public ushort wSecond;
        public ushort wMilliseconds;
    }
    #endregion

    #region enums
    internal enum ReportStatus
    {
        NotSupported = 0,
        Error        = 1,
        AccessDenied = 2,
        Initializing = 3,
        Running      = 4
    }

    internal enum DesiredAccuracy
    {
        DefaultAccuracy = 0,
        HighAccuracy
    }
    #endregion

    #region ILocationReport
    /// <summary>
    /// ILocationReport COM interface
    /// </summary>
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("C8B7F7EE-75D0-4DB9-B62D-7A0F369CA456")]
    internal interface ILocationReport
    {
        [PreserveSig]
        Int32 GetSensorID(ref Guid sensorId);
        [PreserveSig]
        Int32 GetTimestamp(out SYSTEMTIME timestamp);
        [PreserveSig]
        Int32 GetValue([In] ref PROPERTYKEY propertyKey, [Out] PROPVARIANT value);
    }
    #endregion

    #region ILatLongReport
    /// <summary>
    /// ILatLongReport COM interface
    /// </summary>
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("7FED806D-0EF8-4F07-80AC-36A0BEAE3134")]
    internal interface ILatLongReport : ILocationReport
    {
        [PreserveSig]
        new Int32 GetSensorID(ref Guid sensorId);
        [PreserveSig]
        new Int32 GetTimestamp(out SYSTEMTIME timestamp);
        [PreserveSig]
        new Int32 GetValue([In] ref PROPERTYKEY propertyKey, [Out] PROPVARIANT value);
        [PreserveSig]
        Int32 GetLatitude(out double latitude);
        [PreserveSig]
        Int32 GetLongitude(out double longitude);
        [PreserveSig]
        Int32 GetErrorRadius(out double errorRadius);
        [PreserveSig]
        Int32 GetAltitude(out double altitude);
        [PreserveSig]
        Int32 GetAltitudeError(out double altitudeError);
    }
    #endregion

    #region ILocationEvents
    /// <summary>
    /// ILocationEvent COM interface
    /// </summary>
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("CAE02BBF-798B-4508-A207-35A7906DC73D")]
    internal interface ILocationEvents
    {
#if !SILVERLIGHT
        void OnPositionChanged([In] ref Guid reportType, [MarshalAs(UnmanagedType.Interface)] ILocationReport locationReport);
#else
        void OnPositionChanged([In] ref Guid reportType, ILocationReport locationReport);
#endif
        void OnPositionStatusChanged([In] ref Guid reportType, [In] ReportStatus newStatus);
    }
    #endregion

    #region ILocation
    /// <summary>
    /// ILocation COM interface
    /// </summary>
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("AB2ECE69-56D9-4F28-B525-DE1B0EE44237")]
    internal interface ILocation
    {
        [PreserveSig]
#if !SILVERLIGHT
        Int32 RegisterForReport([In, MarshalAs(UnmanagedType.Interface)] ILocationEvents pEvents, [In] ref Guid reportType, [In] uint dwMinReportInterval);
#else
        Int32 RegisterForReport([In] ILocationEvents pEvents, [In] ref Guid reportType, [In] uint dwMinReportInterval);
#endif
        [PreserveSig]
        Int32 UnregisterForReport([In] ref Guid reportType);
        [PreserveSig]
#if !SILVERLIGHT
        Int32 GetReport([In] ref Guid reportType, [MarshalAs(UnmanagedType.Interface)] out ILocationReport locationReport);
#else
        Int32 GetReport([In] ref Guid reportType, out ILocationReport locationReport);
#endif
        [PreserveSig]
        Int32 GetReportStatus([In] ref Guid reportType, ref ReportStatus reportStatus);
        [PreserveSig]
        Int32 GetReportInterval([In] ref Guid reportType, ref UInt32 reportInterval);
        [PreserveSig]
        Int32 SetReportInterval([In] ref Guid reportType, [In] uint milliseconds);
        [PreserveSig]
        Int32 GetDesiredAccuracy([In] ref Guid reportType, ref DesiredAccuracy desiredAccuracy);
        [PreserveSig]
        Int32 SetDesiredAccuracy([In] ref Guid reportType, [In] DesiredAccuracy desiredAccuracy);
        [PreserveSig]
        Int32 RequestPermissions([In] IntPtr hParent, [In] ref Guid pReportTypes, [In] uint count, [In] int fModal);
    }
    #endregion

    #region Location coclass
    /// <summary>
    /// COM wrapper for Location object
    /// </summary>
    [ComImport, GuidAttribute("E5B8E079-EE6D-4E33-A438-C87F2E959254"), ClassInterfaceAttribute(ClassInterfaceType.None)]
    internal class COMLocation
    {
    }
    #endregion
}
