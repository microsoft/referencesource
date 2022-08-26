#pragma once 

#ifndef __INTEROPWIN32SPLAPITHUNK_HPP__
#define __INTEROPWIN32SPLAPITHUNK_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name: 

        InteropWin32ApiThunk.hpp
        
    Abstract:        
        
        PInvoke methods definition.

    Author: 

        Adina Trufinescu (adinatru) April 24th 2003
                                                                             
    Revision History:  
--*/

using namespace System::Windows::Xps::Serialization;
using namespace System::Runtime::InteropServices;

namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
namespace Win32ApiThunk
{   
    ///<SecurityNote>
    /// Critical    - Win32 Print API calls which enables printing and print queue/server management
    ///</SecurityNote>
    [System::Security::SuppressUnmanagedCodeSecurityAttribute]
    [System::Security::SecurityCritical(System::Security::SecurityCriticalScope::Everything)]
    private ref class UnsafeNativeMethods abstract
    {
        public:

        ///<SecurityNote>
        /// Critical    - SUC applied; enables OpenPrinterW API call. 
        ///             - callers must demand AllPrinting if caller requests admin privileges
        ///             - otherwise callers must demand DefaultPrinting
        ///             - cannot be called in Partial Trust
        ///</SecurityNote>        
        [DllImportAttribute("winspool.drv",EntryPoint="OpenPrinterW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeOpenPrinter(String^, IntPtr*, PrinterDefaults^);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetPrinterW API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///               
        ///</SecurityNote>        
        [DllImportAttribute("winspool.drv",EntryPoint="GetPrinterW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeGetPrinter(IntPtr, UInt32, SafeMemoryHandle^, UInt32, UInt32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetPrinterData API call. 
        ///             - non-admin operation; callers must demand DefaultPrinting      
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="GetPrinterDataW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        UInt32
        InvokeGetPrinterData(IntPtr, String^, UInt32*, SafeMemoryHandle^, UInt32, UInt32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetPrinterDriverW API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>        
        [DllImportAttribute("winspool.drv",EntryPoint="GetPrinterDriverW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeGetPrinterDriver(IntPtr, String^, UInt32, SafeMemoryHandle^, UInt32, UInt32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables EnumPrintersW API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="EnumPrintersW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeEnumPrinters(UInt32, String^, UInt32, SafeMemoryHandle^, UInt32, UInt32*, UInt32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables ClosePrinterW API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="ClosePrinter",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        [System::Runtime::ConstrainedExecution::ReliabilityContract(System::Runtime::ConstrainedExecution::Consistency::WillNotCorruptState,
                                                                    System::Runtime::ConstrainedExecution::Cer::Success)]
        static
        bool
        InvokeClosePrinter(IntPtr);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables AddPrinterConnection API call in Intranet Zone  
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="AddPrinterConnectionW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeAddPrinterConnection(String^);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables DeletePrinterConnection API call in Intranet Zone  
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="DeletePrinterConnectionW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeDeletePrinterConnection(String^);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetDefaultPrinterW API call in Intranet Zone
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="GetDefaultPrinterW",
                            CharSet=CharSet::Unicode,
                            SetLastError=true, 
                            CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeGetDefaultPrinter(System::Text::StringBuilder^, int*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetJob API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="GetJobW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeGetJob(IntPtr, UInt32, UInt32, SafeMemoryHandle^, UInt32, UInt32*);


        ///<SecurityNote>
        /// Critical    - SUC applied; enables SetJob API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="SetJobW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeSetJob(IntPtr, UInt32, UInt32, IntPtr, UInt32);


        ///<SecurityNote>
        /// Critical    - SUC applied; enables EnumJobs API call in Intranet Zone
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="EnumJobsW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeEnumJobs(IntPtr, UInt32, UInt32, UInt32, SafeMemoryHandle^, UInt32, UInt32*, UInt32*);

        #ifdef XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - SUC applied; enables AddJob API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv", EntryPoint = "AddJob",
                            CharSet = CharSet::Unicode,
                            SetLastError = true,
                            CallingConvention = CallingConvention::Winapi)]

        static
        bool
        InvokeAddJob(IntPtr, UInt32, SafeMemoryHandle^, UInt32, UInt32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables AddJob API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv", EntryPoint = "ScheduleJob",
                            CharSet = CharSet::Unicode,
                            SetLastError = true,
                            CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeScheduleJob(IntPtr, UInt32);
        
        ///<SecurityNote>
        /// Critical    - SUC applied; enables WritePrinter API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint="EDocWritePrinter",
                   CharSet=CharSet::Unicode,
                   SetLastError=true, 
                   CallingConvention = CallingConvention::Winapi)]
        static 
        Boolean
        InvokeEDocWritePrinter(IntPtr, IntPtr, Int32, Int32*);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables FlushPrinter API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint="FlushPrinter",
                   CharSet=CharSet::Unicode,
                   SetLastError=true, 
                   CallingConvention = CallingConvention::Winapi)]
        static 
        Boolean
        InvokeFlushPrinter(IntPtr, IntPtr, Int32, Int32*, Int32);

        #endif //XPSJOBNOTIFY

        [DllImportAttribute("winspool.drv",EntryPoint="ReportJobProcessingProgress",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        Int32
        InvokeReportJobProgress(IntPtr, Int32, Int32, Int32);

        [DllImportAttribute("winspool.drv",EntryPoint="StartPagePrinter",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeStartPagePrinter(IntPtr);

        [DllImportAttribute("winspool.drv",EntryPoint="EndPagePrinter",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeEndPagePrinter(IntPtr);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables SetDefaultPrinterW API call in Intranet Zone 
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="SetDefaultPrinterW",
                            CharSet=CharSet::Unicode,
                            SetLastError=true, 
                            CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeSetDefaultPrinter(String^);
        
        ///<SecurityNote>
        /// Critical    - SUC applied; enables StartDocPrinter API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "StartDocPrinter",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        Int32
        InvokeStartDocPrinter(IntPtr, Int32, DocInfoThree^);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables EndDocPrinter API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "EndDocPrinter",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeEndDocPrinter(IntPtr);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables AbortPrinter API call in Intranet Zone    
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "AbortPrinter",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeAbortPrinter(IntPtr);


        ///<SecurityNote>
        /// Critical    - SUC applied; enables GetSpoolFileHandle API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "GetSpoolFileHandle",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        IntPtr
        InvokeGetSpoolFileHandle(IntPtr);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables CommitSpoolData API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "CommitSpoolData",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        IntPtr
        InvokeCommitSpoolData(IntPtr, Microsoft::Win32::SafeHandles::SafeFileHandle^, Int32);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables CloseSpoolFileHandle API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "CloseSpoolFileHandle",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeCloseSpoolFileHandle(IntPtr, Microsoft::Win32::SafeHandles::SafeFileHandle^);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables DocumentEvent API call in Intranet Zone   
        ///             - non-admin operation; callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("winspool.drv", EntryPoint = "DocumentEvent",
                   CharSet = CharSet::Unicode,
                   SetLastError = true,
                   CallingConvention = CallingConvention::Winapi)]
        static
        Int32
        InvokeDocumentEvent(IntPtr, IntPtr, Int32, UInt32, SafeHandle^, UInt32, SafeMemoryHandle^);

        ///<SecurityNote>
        /// Non-Critical  - no SUC applied. Enabled only in Full Trust
        ///               - regardless, callers demand AllPrinting since this is an admin only scenario
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="SetPrinterDataW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        UInt32
        InvokeSetPrinterDataIntPtr(IntPtr, String^, UInt32, IntPtr, UInt32);

        ///<SecurityNote>
        /// Non-Critical - no SUC applied. Enabled only in Full Trust
        ///              - regardless, callers demand AllPrinting since this is an admin only scenario
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="SetPrinterDataW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        UInt32
        InvokeSetPrinterDataInt32(IntPtr, String^, UInt32, Int32*, UInt32);

        ///<SecurityNote>
        /// Non-Critical - no SUC applied. Enabled only in Full Trust
        ///              - regardless, callers demand AllPrinting since this is an admin only scenario
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="AddPrinterW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        HANDLE*
        InvokeAddPrinter(String^, UInt32, SafeMemoryHandle^);
        
        ///<SecurityNote>
        /// Non-Critical - no SUC applied. Enabled only in Full Trust
        ///              - regardless, callers demand AllPrinting since this is an admin only scenario
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="SetPrinterW",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeSetPrinter(IntPtr, UInt32, SafeMemoryHandle^, UInt32);

        ///<SecurityNote>
        /// Non-Critical - no SUC applied. Enabled only in Full Trust
        ///              - regardless, callers demand AllPrinting since this is an admin only scenario
        ///</SecurityNote>
        [DllImportAttribute("winspool.drv",EntryPoint="DeletePrinter",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        bool
        InvokeDeletePrinter(IntPtr);
        
        ///<SecurityNote>
        /// Critical    - SUC applied; enables Win32 GetComputerName API call in Intranet Zone
        ///             - callers must demand DefaultPrinting
        ///</SecurityNote>
        [DllImport("Kernel32.dll", EntryPoint="GetComputerNameW",
                   CharSet=CharSet::Unicode,
                   SetLastError=true, 
                   CallingConvention = CallingConvention::Winapi)]
        static 
        bool 
        GetComputerName(System::Text::StringBuilder^ nameBuffer, int* bufferSize);
    };
    
    ///<SecurityNote>
    /// Critical    - Win32 Print API calls which enables printing and print queue/server management
    ///</SecurityNote>
    [System::Security::SuppressUnmanagedCodeSecurityAttribute]
    [System::Security::SecurityCritical(System::Security::SecurityCriticalScope::Everything)]
    private ref class PresentationNativeUnsafeNativeMethods abstract
    {
    public:
        ///<SecurityNote>
        /// Critical    - Calls critical IsStartXpsPrintJobSupportedImpl SUC applied; enables LateBoundStartXpsPrintJob API call. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        static
        BOOL
        IsStartXpsPrintJobSupported(VOID) 
        {
            LoadPresentationNative();
            return IsStartXpsPrintJobSupportedImpl();
        }
        
        ///<SecurityNote>
        /// Critical    - Calls critical LateBoundStartXpsPrintJobImpl; SUC applied; enables LateBoundStartXpsPrintJob API call. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        static
        UInt32
        LateBoundStartXpsPrintJob( 
            String^ printerName,
            String^ jobName,
            String^ outputFileName,
            Microsoft::Win32::SafeHandles::SafeWaitHandle^ progressEvent,
            Microsoft::Win32::SafeHandles::SafeWaitHandle^ completionEvent,
            UINT8  *printablePagesOn,
            UINT32 printablePagesOnCount,
            VOID **xpsPrintJob,
            VOID **documentStream,
            VOID **printTicketStream
        )
        {
            LoadPresentationNative();
            
            return LateBoundStartXpsPrintJobImpl(
                printerName, 
                jobName, 
                outputFileName, 
                progressEvent, 
                completionEvent, 
                printablePagesOn,
                printablePagesOnCount,
                xpsPrintJob,
                documentStream,
                printTicketStream);
        }
        
        ///<SecurityNote>
        /// Critical    - Calls critical IsPrintPackageTargetSupportedImpl; SUC applied; enables Print Document Package API Interfaces. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        static
        BOOL
        IsPrintPackageTargetSupported(VOID)
        {
            LoadPresentationNative();
            return IsPrintPackageTargetSupportedImpl();
        }

        ///<SecurityNote>
        /// Critical    - Calls critical PrintToPackageTargetImpl; SUC applied; enables Print Document Package API Interfaces. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        static
            UInt32
        PrintToPackageTarget(
            String^ printerName,
            String^ jobName,
            ComTypes::IStream^ jobPrintTicketStream,
            [Out] RCW::IPrintDocumentPackageTarget^ %printDocPackageTarget,
            [Out] RCW::IXpsDocumentPackageTarget^ %xpsPackageTarget
        )
        {
            LoadPresentationNative();

            return PrintToPackageTargetImpl(
                printerName,
                jobName,
                jobPrintTicketStream,
                printDocPackageTarget,
                xpsPackageTarget);
        }
    private:
        ///<SecurityNote>
        /// Critical    - SUC applied; enables LateBoundStartXpsPrintJob API call. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        [DllImportAttribute("PresentationNative_v0400.dll" ,EntryPoint="IsStartXpsPrintJobSupported",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        BOOL
        IsStartXpsPrintJobSupportedImpl(VOID);
        
        ///<SecurityNote>
        /// Critical    - SUC applied; enables LateBoundStartXpsPrintJob API call. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        [DllImportAttribute("PresentationNative_v0400.dll" ,EntryPoint="LateBoundStartXpsPrintJob",
                             CharSet=CharSet::Unicode,
                             SetLastError=true, 
                             CallingConvention = CallingConvention::Winapi)]
        static
        UInt32
        LateBoundStartXpsPrintJobImpl( 
            String^ printerName,
            String^ jobName,
            String^ outputFileName,
            Microsoft::Win32::SafeHandles::SafeWaitHandle^ progressEvent,
            Microsoft::Win32::SafeHandles::SafeWaitHandle^ completionEvent,
            UINT8  *printablePagesOn,
            UINT32 printablePagesOnCount,
            VOID **xpsPrintJob,
            VOID **documentStream,
            VOID **printTicketStream
        );
        
        ///<SecurityNote>
        /// Critical    - SUC applied; enables Print Document Package API Interfaces. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        [DllImportAttribute("PresentationNative_v0400.dll", EntryPoint = "IsPrintPackageTargetSupported",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
        static
        BOOL
        IsPrintPackageTargetSupportedImpl(VOID);

        ///<SecurityNote>
        /// Critical    - SUC applied; enables Print Document Package API Interfaces. 
        ///             - otherwise callers must demand DefaultPrinting
        ///</SecurityNote>        
        [DllImportAttribute("PresentationNative_v0400.dll", EntryPoint = "PrintToPackageTarget",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
        static
        PrintToPackageTargetImpl(
            String^ printerName,
            String^ jobName,
            ComTypes::IStream^ jobPrintTicketStream,
            [MarshalAs(UnmanagedType::Interface)]
            [Out] RCW::IPrintDocumentPackageTarget^ %printDocPackageTarget,
            [MarshalAs(UnmanagedType::Interface)]
            [Out] RCW::IXpsDocumentPackageTarget^ %xpsPackageTarget
        );


        static void LoadPresentationNative()
        {
            static bool isPresentationNativeLoaded = false;
            
            if (!isPresentationNativeLoaded)
            {
                MS::Internal::NativeWPFDLLLoader::LoadPresentationNative();
                isPresentationNativeLoaded = true;
            }
        }
        
    };

}
}
}
}
#endif

