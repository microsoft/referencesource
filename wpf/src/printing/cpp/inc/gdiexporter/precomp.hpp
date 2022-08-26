#ifndef GDIEXPORTER

#define GDIEXPORTER

#define INITGUID

#ifndef METRODEVICE

#include <windows.h>
#include <wtypes.h>

#endif

#using REACHFRAMEWORK_DLL   as_friend
#using PRESENTATIONCORE_DLL as_friend

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::Runtime;
using namespace System::Runtime::ConstrainedExecution;
using namespace System::Runtime::InteropServices;
using namespace System::Security;
using namespace System::Security::Permissions;
using namespace System::Text;
using namespace System::Windows;
using namespace System::Windows::Media;

using namespace Microsoft::Internal::AlphaFlattener;

#include "..\LegacyDevice.hpp"

using namespace System::Printing;
                
namespace Microsoft { namespace Internal { namespace GDIExporter
{

value class PointI
{
public:
    int     x;
    int     y;
};


ref class UnsafeNativeMethods abstract
{
internal:

    /// <SecurityNote>
    ///     Critical   : Calling unmanaged code
    /// </SecurityNote>
    [System::Runtime::InteropServices::DllImport(
            "gdi32.dll",
            EntryPoint = "DeleteObject",
            SetLastError = true,
            CallingConvention = System::Runtime::InteropServices::CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    DeleteObject(
        HGDIOBJ hObject   // handle to graphic object
        );
    

    /// <SecurityNote>
    ///     Critical   : Calling unmanaged code
    /// </SecurityNote>
    [InteropServices::DllImport(
            "gdi32.dll",
            EntryPoint = "DeleteDC",
            SetLastError = true,
            CallingConvention = InteropServices::CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    DeleteDC(
        HGDIOBJ hObject   // handle to graphic object
        );
    

    /// <SecurityNote>
    ///     Critical   : Calling unmanaged code
    /// </SecurityNote>
    [InteropServices::DllImport(
            "gdi32.dll",
            EntryPoint = "RemoveFontMemResourceEx",
            SetLastError = true,
            CallingConvention = InteropServices::CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    RemoveFontMemResourceEx(
        HANDLE hFont   // handle to graphic object
        );
};    


///<SecurityNote>
/// Critical    - Base class is SecurityCritical
///</SecurityNote>
[SecurityCritical]
ref class GdiSafeHandle : public InteropServices::SafeHandle
{
public:
    GdiSafeHandle() : InteropServices::SafeHandle(IntPtr::Zero, true) 
    { 
    }

    property bool IsInvalid
    {
        ///<SecurityNote>
        /// Critical    - Type is critical
        ///</SecurityNote>
        [SecurityCritical]
        [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
        bool virtual get() override { return IsClosed || (handle == IntPtr::Zero); }
    }

protected:
    /// <SecurityNote>
    ///     Critical   : Calls native method to delete GDI object
    /// </SecurityNote>
    [SecurityCritical]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    bool virtual ReleaseHandle() override
    {
        IntPtr tempHandle = handle;
        handle = IntPtr::Zero;
        
        if (tempHandle != IntPtr::Zero)
        {
            return UnsafeNativeMethods::DeleteObject((HGDIOBJ) tempHandle) != 0;
        }

        return true;
    }
};


///<remarks>
///  SafeHandle that wraps GDI device contexts
///</remarks>
///<SecurityNote>
/// Critical    - Base class is SecurityCritical
///</SecurityNote>
[SecurityCritical]
ref class GdiSafeDCHandle : public GdiSafeHandle
{
public:
    GdiSafeDCHandle() : GdiSafeHandle() 
    { 
    }

#ifdef DBG
    [SecurityCritical]    
    inline HDC GetHDC()
    {
        return (HDC) (void *) handle;
    }
#endif

protected:
    ///<SecurityNote>
    /// Critical    - Calls critical method to delete GDI device context
    ///</SecurityNote>
    [SecurityCritical]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    bool virtual ReleaseHandle() override
    {
        IntPtr tempHandle = handle;
        handle = IntPtr::Zero;
        
        if (tempHandle != IntPtr::Zero)
        {
            return UnsafeNativeMethods::DeleteDC((HGDIOBJ) tempHandle) != 0;
        }

        return true;
    }
};

///<remarks>
///  SafeHandle that wraps GDI font resources.
///</remarks>
///<SecurityNote>
/// Critical    - Base class is SecurityCritical
///</SecurityNote>
[SecurityCritical]
ref class GdiFontResourceSafeHandle : public System::Runtime::InteropServices::SafeHandle
{
public:
     ///<SecurityNote>
     /// Critical    - Changes critical state
     /// TreatAsSafe - Does not expose critical state, critical state cannot be influenced be external inputs
     ///</SecurityNote>
     [SecuritySafeCritical]
    GdiFontResourceSafeHandle() : System::Runtime::InteropServices::SafeHandle(IntPtr::Zero, true) 
    { 
        m_timeStamp = DateTime::Now;
    }

    property bool IsInvalid
    {
        ///<SecurityNote>
        /// Critical    - Type is critical
        ///</SecurityNote>
        [SecurityCritical]
        [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
        bool virtual get() override { return IsClosed || (handle == IntPtr::Zero); }
    }

    property DateTime TimeStamp
    {
        ///<SecurityNote>
        /// Critical    - Returns critical data
        ///</SecurityNote>
        [SecurityCritical]
        DateTime get()
        {
            return m_timeStamp;
        }
    }

protected:
    ///<SecurityNote>
    /// Critical    - Calls critical method to uninstall memory font
    ///</SecurityNote>
    [SecurityCritical]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    bool virtual ReleaseHandle() override
    {
        IntPtr tempHandle = handle;
        handle = IntPtr::Zero;
        
        if (tempHandle != IntPtr::Zero)
        {            
            return UnsafeNativeMethods::RemoveFontMemResourceEx((HANDLE)tempHandle) != 0;            
        }

        return true;
    }

    /// <SecurityNote>
    ///     Critical   : Records creation time of handle - used to determine lifetime of handle
    /// </SecurityNote>
    [SecurityCritical]
    DateTime m_timeStamp;
};


///////////////////////////////////////////////////////////////////////////////////////////////////

// External interface for printing code
#include "nativemethods.h"
#include "utils.h"
#include "printmsg.h"

#include "gdidevice.h"

#include "gdipath.h"
#include "gdibitmap.h"
#include "gdirt.h"

}}}

#endif
