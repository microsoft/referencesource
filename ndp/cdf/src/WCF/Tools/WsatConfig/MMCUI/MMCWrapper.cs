//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;

    struct PSM
    {
        internal const int SETCURSEL = (0x0400 + 101);
        internal const int REMOVEPAGE = (0x0400 + 102);
        internal const int ADDPAGE = (0x0400 + 103);
        internal const int CHANGED = (0x0400 + 104);
        internal const int UNCHANGED = (0x0400 + 109);
        internal const int SETWIZBUTTONS = (0x0400 + 112);
    }

    struct PSN
    {
        internal const int FIRST = unchecked((0 - 200));
        internal const int LAST = unchecked((0 - 299));
        internal const int SETACTIVE = unchecked((FIRST - 0));
        internal const int KILLACTIVE = unchecked((FIRST - 1));
        internal const int APPLY = unchecked((FIRST - 2));
        internal const int RESET = unchecked((FIRST - 3));
        internal const int HELP = unchecked((FIRST - 5));
        internal const int WIZBACK = unchecked((FIRST - 6));
        internal const int WIZNEXT = unchecked((FIRST - 7));
        internal const int WIZFINISH = unchecked((FIRST - 8));
        internal const int QUERYCANCEL = unchecked((FIRST - 9));
        internal const int GETOBJECT = unchecked((FIRST - 10));
    }

    struct WM
    {
        internal const uint INITDIALOG = 0x0110;
        internal const uint COMMAND = 0x0111;
        internal const uint DESTROY = 0x0002;
        internal const uint NOTIFY = 0x004E;
        internal const uint PAINT = 0x000F;
        internal const uint SETFOCUS = 0x0007;
        internal const uint SHOWWINDOW = 0x0018;
    }

    struct PSP
    {
        internal const int DEFAULT = 0x00000000;
        internal const int DLGINDIRECT = 0x00000001;
        internal const int USEHICON = 0x00000002;
        internal const int USEICONID = 0x00000004;
        internal const int USETITLE = 0x00000008;
        internal const int RTLREADING = 0x00000010;

        internal const int HASHELP = 0x00000020;
        internal const int USEREFPARENT = 0x00000040;
        internal const int USECALLBACK = 0x00000080;
        internal const int PREMATURE = 0x00000400;

        internal const int HIDEHEADER = 0x00000800;
        internal const int USEHEADERTITLE = 0x00001000;
        internal const int USEHEADERSUBTITLE = 0x00002000;
    }

    struct HRESULT
    {
        internal const int S_OK = 0;
        internal const int S_FALSE = 1;
        internal const int E_FAIL = unchecked((int)0x80004005);
        internal const int E_NOTIMPL = unchecked((int)0x80004001);
        internal const int E_ACCESSDENIED = unchecked((int)0x80070005);
    }

    struct PSNRET
    {
        internal const long NOERROR = 0;
        internal const long INVALID = 1;
        internal const long INVALID_NOCHANGEPAGE = 2;
        internal const long MESSAGE_HANDLED = 3;
    }

    struct DS
    {
        internal const int SETFONT = 0x40;
        internal const int FIXEDSYS = 0x0008;
        internal const int DS_ABSALIGN = 0x01;
        internal const int DS_SYSMODAL = 0x02;
        internal const int DS_LOCALEDIT = 0x20;   /* Edit items get Local storage. */
        internal const int DS_SETFONT = 0x40;   /* User specified font for Dlg controls */
        internal const int DS_MODALFRAME = 0x80;   /* Can be combined with WS_CAPTION  */
        internal const int DS_NOIDLEMSG = 0x100;  /* WM_ENTERIDLE message will not be sent */
        internal const int DS_SETFOREGROUND = 0x200;  /* not in win3.1 */

        internal const int DS_3DLOOK = 0x0004;
        internal const int DS_FIXEDSYS = 0x0008;
        internal const int DS_NOFAILCREATE = 0x0010;
        internal const int DS_CONTROL = 0x0400;
        internal const int DS_CENTER = 0x0800;
        internal const int DS_CENTERMOUSE = 0x1000;
        internal const int DS_CONTEXTHELP = 0x2000;

        internal const int DS_SHELLFONT = (DS_SETFONT | DS_FIXEDSYS);
        internal const int DS_USEPIXELS = 0x8000;

    }

    [StructLayout(LayoutKind.Sequential)]
    struct NMHDR
    {
        internal IntPtr hwndFrom;
        internal UIntPtr idFrom;
        internal uint code;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    struct PropSheetPage
    {
        internal int dwSize;
        internal int dwFlags;
        internal IntPtr hInstance;

        // This is a union of the following Data items
        // String          pszTemplate; 
        internal SafeLocalAllocation pResource;

        // This is a union of the following Data items
        // IntPtr          hIcon; 
        // String          pszIcon; 
        internal IntPtr hIcon;  // This must be IntPtr.Zero or a SafeHandle should be used

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pszTitle;
        internal DialogProc pfnDlgProc;
        internal IntPtr longParameter;

        internal PropSheetPageProc pfnCallback;
        internal IntPtr pcRefParent;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pszHeaderTitle;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string pszHeaderSubTitle;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 2, CharSet = CharSet.Auto)]
    struct DialogTemplate
    {
        internal uint style;
        internal uint dwExtendedStyle;
        internal ushort cdit;
        internal short x;
        internal short y;
        internal short cx;
        internal short cy;
        // DialogTemplate is a varialbe-length structure
        // The following 3 fields will be length of 3 sub-arrays and they'll be zeroes in this app
        internal short wMenuResource;
        internal short wWindowClass;
        internal short wTitleArray;
    }

    [CLSCompliantAttribute(false)]
    public delegate bool DialogProc(IntPtr windowDialog, UInt32 message, IntPtr wordParameter, IntPtr longParameter);

    [CLSCompliantAttribute(false)]
    public delegate int PropSheetPageProc(IntPtr window, int message, IntPtr longParameter);

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("85DE64DD-EF21-11cf-A285-00C04FD8DBE6")]
    public interface IPropertySheetCallback
    {
        [PreserveSig()]
        int AddPage(SafePropertyPage prop);

        [PreserveSig()]
        int RemovePage(IntPtr prop);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("85DE64DC-EF21-11cf-A285-00C04FD8DBE6")]
    public interface IExtendPropertySheet
    {
        [PreserveSig()]
        int CreatePropertyPages(IPropertySheetCallback provider, IntPtr handle, IDataObject dataObject);

        [PreserveSig()]
        int QueryPagesFor(IDataObject dataObject);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("B7A87232-4A51-11D1-A7EA-00C04FD909DD")]
    public interface IExtendPropertySheet2
    {
        [PreserveSig()]
        int CreatePropertyPages(IPropertySheetCallback provider, IntPtr handle, IDataObject dataObject);

        [PreserveSig()]
        int QueryPagesFor(IDataObject dataObject);

        [PreserveSig()]
        int GetWatermarks(IDataObject dataObject, ref IntPtr watermark, ref IntPtr header, ref IntPtr palette, ref int stretch);
    }
}
