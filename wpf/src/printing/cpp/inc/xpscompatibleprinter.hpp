#pragma once

#ifndef __XPSCOMPATIBLEPRINTER_HPP__
#define __XPSCOMPATIBLEPRINTER_HPP__
/*++

    Copyright (C) 2002 - 2016 Microsoft Corporation
    All rights reserved.

    Module Name:

        XpsCompatiblePrinter.hpp

        Abstract:

        Managed wrapper for Print Document Package API Interfaces.

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
    using namespace System::Windows::Xps::Serialization;

    private ref class XpsCompatiblePrinter 
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
        XpsCompatiblePrinter(
            String^ printerName
        );

        ~XpsCompatiblePrinter();

        ///<SecurityNote>
        /// Critical    - Calls into the native methods to start a print job
        /// TreatAsSafe - this class demands DefaultPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        void
        StartDocPrinter(
            DocInfoThree^         docInfo,
            PrintTicket^ printTicket,
            bool mustSetPrintJobIdentifier
            );

        ///<SecurityNote>
        /// Critical    - Releases printer resources
        /// TreatAsSafe - This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        void
        EndDocPrinter(
            void
            );


        ///<SecurityNote>
        /// Critical    - Calls Cancel on critical member printDocPackageTarget
        /// TreatAsSafe - This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        virtual
        void
        AbortPrinter(
            void
            );

        
        property
        virtual 
        int
        JobIdentifier
        {
            ///<SecurityNote>
            /// Critical    - Critical method used to obtain a print job identifier
            /// TreatAsSafe - This class demands DefaultPrinting.
            ///</SecurityNote>
            [SecuritySafeCritical]
            [System::Drawing::Printing::PrintingPermission(
             SecurityAction::Demand,
             Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
            int get();
        }    

        property
        RCW::IXpsDocumentPackageTarget^
        XpsPackageTarget
        {
            ///<SecurityNote>
            /// Critical    - Exposes critical member xpsPackageTarget
            ///</SecurityNote>
            [SecurityCritical]
            [System::Drawing::Printing::PrintingPermission(
             SecurityAction::Demand,
             Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
            RCW::IXpsDocumentPackageTarget^ get();
        }

        property
        RCW::IXpsOMPackageWriter^
        XpsOMPackageWriter
        {
            ///<SecurityNote>
            /// Critical    - Exposes critical member xpsPackageTarget
            ///</SecurityNote>
            [SecurityCritical]
            [System::Drawing::Printing::PrintingPermission(
                SecurityAction::Demand,
                Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
            void set(RCW::IXpsOMPackageWriter^ packageWriter);
        }

        private:
            ///<SecurityNote>
            /// Critical    - Used to access a printer device
            ///</SecurityNote>
            [SecurityCritical]
            String^ _printerName;

            ///<SecurityNote>
            /// Critical    - COM interface used to cancel a print job
            ///</SecurityNote>
            [SecurityCritical]
            RCW::IPrintDocumentPackageTarget^ _printDocPackageTarget;

            ///<SecurityNote>
            /// Critical    - COM interface used to get the package writer for content serialization
            ///</SecurityNote>
            [SecurityCritical]
            RCW::IXpsDocumentPackageTarget^ _xpsPackageTarget;

            ///<SecurityNote>
            /// Critical    - COM interface used to get the print job ID
            ///</SecurityNote>
            [SecurityCritical]
            RCW::PrintDocumentPackageStatusProvider^ _xpsPackageStatusProvider;

            ///<SecurityNote>
            /// Critical    - COM interface used to definitely end the print job
            ///</SecurityNote>
            [SecurityCritical]
            RCW::IXpsOMPackageWriter^ _packageWriter;
    };
}
}
}
#endif

