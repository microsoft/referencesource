#pragma once

#ifndef __INTEROPPRINTERHANDLERBASE_HPP__
#define __INTEROPPRINTERHANDLERBASE_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2011 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropPrinterHandlerBase.hpp
        
    Abstract:

        Interface for Win32 print APIs. This interface wraps a printer handle
        and does gets, sets and enum operations. Implements IDisposable. The caller
        must call Dispose when done using the object.
--*/
namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
    using namespace System::IO;
    using namespace System::Security;
    using namespace System::Security::Permissions;
    using namespace System::Runtime::InteropServices;

    ///<Summary>
    /// Abstract interface to native handle based printer API's
    ///</Summary>
    ///<SecurityNote>
    /// Critical    - Derives from critical SafeHandle
    ///</SecurityNote>
    [SecurityCritical]
    private ref class PrinterThunkHandlerBase abstract : public System::Runtime::InteropServices::SafeHandle 
    {
        protected:
            
        ///<SecurityNote>
        /// Critical    - stores a win32 print handle
        ///</SecurityNote>
        [SecurityCritical]
        PrinterThunkHandlerBase(
            void
        ) : SafeHandle(IntPtr::Zero, true)
        {            
        }
        
        public:
            
        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to initialize a printer device for printing
        /// TreatAsSafe - this class demands DefaultPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual 
        Int32
        ThunkStartDocPrinter(
            DocInfoThree^         docInfo,
            PrintTicket^ printTicket
            ) = 0;

        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to end a print job
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        Boolean
        ThunkEndDocPrinter(
            void
            ) = 0;


        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to cancel a print job
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        Boolean
        ThunkAbortPrinter(
            void
            ) = 0;

        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to create a stream for spooling a print job
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual 
        void
        ThunkOpenSpoolStream(
            void
            ) = 0;

        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to commit spooled data to a print job
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual 
        void
        ThunkCommitSpoolData(
            Int32                   bytes
            ) = 0;

        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to close the spool stream for print job
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual 
        Boolean
        ThunkCloseSpoolStream(
            void
            ) = 0;

        ///<SecurityNote>
        /// Critical    - Implementations are expected to call native methods to report job spooling progress to a printer driver
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        Int32
        ThunkReportJobProgress(
            Int32                                                           jobId,
            JobOperation                                                    jobOperation,
            System::Windows::Xps::Packaging::PackagingAction                packagingAction
            ) = 0;

        property
        virtual 
        int
        JobIdentifier
        {
            ///<SecurityNote>
            /// Critical    - Overrides critical method used to obtain a print job identifier
            /// TreatAsSafe - this class demands DefaultPrinting.
            ///</SecurityNote>
            [SecuritySafeCritical]
            [System::Drawing::Printing::PrintingPermission(
             SecurityAction::Demand,
             Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
            virtual int get() = 0;
        }    

        property
        virtual 
        Stream^
        SpoolStream
        {
            ///<SecurityNote>
            /// Critical    - Stream used to send raw data to printer device
            ///</SecurityNote>
            [SecurityCritical]
            virtual Stream^ get() = 0;
        }    
    };

}
}
}
#endif

