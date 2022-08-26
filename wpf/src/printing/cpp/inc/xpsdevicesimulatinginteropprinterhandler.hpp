#pragma once

#ifndef __XPSDEVSIMINTEROPPRINTERHANDLER_HPP__
#define __XPSDEVSIMINTEROPPRINTERHANDLER_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2011 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        XpsDeviceSimulatingPrintThunkHandler.hpp
        
    Abstract:

        Managed wrapper for Win32 XPS print APIs. This object wraps a printer handle
        and does gets, sets and enum operations. Implements IDisposable. The caller
        must call Dispose when done using the object.

    Author: 

        Adina Trufinescu (adinatru) April 24th 2003
                                                                             
    Revision History:  
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

    private ref class XpsDeviceSimulatingPrintThunkHandler : public PrinterThunkHandlerBase
    {
        public:
           
        ///<SecurityNote>
        /// Critical    - Sets critical member printerName used to access a printer device to untrusted value
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        XpsDeviceSimulatingPrintThunkHandler(
            String^ printerName
        );
        
        property
        virtual 
        Boolean
        IsInvalid
        {
            ///<SecurityNote>
            /// Critical    - Member of critical type
            /// TreatAsSafe - Does not change critical state
            ///             - Does not expose critical data
            ///</SecurityNote>
            [SecuritySafeCritical]
            Boolean virtual get() override;
        }

        ///<SecurityNote>
        /// Critical    - Member of critical type
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        Boolean 
        ReleaseHandle(
            void
            ) override;

        ///<SecurityNote>
        /// Critical    - Calls native methods to start a print job
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
            ) override;

        ///<SecurityNote>
        /// Critical    - calls native methods to end a print job
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
            ) override;


        ///<SecurityNote>
        /// Critical    - calls native methods to cancel a print job
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
            ) override;

        ///<SecurityNote>
        /// Critical    - Overrides critical method that is supposed to create print job spool stream
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
            ) override;
            
        ///<SecurityNote>
        /// Critical    - Overrides critical method that is supposed to commit spooled data to the printer device
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
            ) override;

        ///<SecurityNote>
        /// Critical    - Overrides critical method that is supposed to close the print jobs spooler stream
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
            ) override;

        ///<SecurityNote>
        /// Critical    - Overrides critical method that is supposed to reprort printing progress to the printer device
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
            ) override;

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
            int get() override;
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
            Stream^ get() override;
        }  

        private:            
            ///<SecurityNote>
            /// Critical    - Used to access a printer device
            ///</SecurityNote>
            [SecurityCritical]
            String^         printerName;

            ///<SecurityNote>
            /// Critical    - Stream used to send raw data to printer device
            ///</SecurityNote>
            [SecurityCritical]
            Stream^         spoolerStream;

            ///<SecurityNote>
            /// Critical    - Native interface used to cancel a print job
            ///</SecurityNote>
            [SecurityCritical]
            IXpsPrintJob*   xpsPrintJob;

            int             jobIdentifier;
    };    
}
}
}
#endif

