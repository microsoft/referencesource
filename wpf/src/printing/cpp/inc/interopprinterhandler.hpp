#pragma once

#ifndef __INTEROPPRINTERHANDLER_HPP__
#define __INTEROPPRINTERHANDLER_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropPrinterHandler.hpp
        
    Abstract:

        Managed wrapper for Win32 print APIs. This object wraps a printer handle
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

    namespace DirectInteropForPrintQueue
    {   
        ref class PrinterInfoTwoSetter;
    }

    ///<remarks>
    ///  SafeHandle that wraps native memory.
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  SafeMemoryHandle : public System::Runtime::InteropServices::SafeHandle 
    {
        public:

        ///<remarks>
        /// Allocates and initializes native memory takes ownership (will free memory in Dispose).
        ///</remarks>
        ///<SecurityNote>
        /// Critical    - Exposes critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        static 
        bool
        TryCreate(
            Int32   byteCount,
            __out SafeMemoryHandle^ % result
            );

        ///<remarks>
        /// Allocates and initializes native memory takes ownership (will free memory in Dispose).
        ///</remarks>
        ///<SecurityNote>
        /// Critical    - Returns critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        static 
        SafeMemoryHandle^
        Create(
            Int32   byteCount
            );

        ///<SecurityNote>
        /// Critical    - Stores an arbitrary HGLOBAL that can later be freed.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle(
            IntPtr   Win32Pointer
            );

        ///<remarks>
        /// Wraps an IntPtr but does not take ownership and does not free handle in Dispose.
        ///</remarks>
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        ///</SecurityNote>
        [SecurityCritical]
        static 
        SafeMemoryHandle^ 
        Wrap(
            IntPtr   Win32Pointer
            );

        property
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
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;

        property
        Int32
        Size
        {
            ///<SecurityNote>
            /// Critical    - Member of critical type
            /// TreatAsSafe - Does not change critical state
            ///             - Does not expose critical data
            ///</SecurityNote>
            [SecuritySafeCritical]
            Int32 virtual get();
        }

        ///<SecurityNote>
        /// Critical    - Writes to arbitrary native memory
        ///</SecurityNote>
        [SecurityCritical]
        void 
        CopyFromArray(
            array<Byte>^ source,
            Int32 startIndex, 
            Int32 length
            );
        
        ///<SecurityNote>
        /// Critical    - Reads from from arbitrary native memory
        ///</SecurityNote>
        [SecurityCritical]
        void 
        CopyToArray(
            array<Byte>^ destination, 
            Int32 startIndex, 
            Int32 length
            );

        static 
        property
        SafeMemoryHandle^ Null 
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            /// TreatAsSafe - Handle is null which is safe.
            ///</SecurityNote>
            [SecuritySafeCritical]
            SafeMemoryHandle^ get();
        }

        private:
        
        ///<SecurityNote>
        /// Critical    - Stores an arbitrary HGLOBAL that can later be freed.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle(
            IntPtr   Win32Pointer,
            Boolean  ownsHandle
            );

        static 
        Exception^ 
        VerifyBufferArguments(
            String^ bufferName,
            array<Byte>^ buffer,
            Int32 startIndex, 
            Int32 length
            );
    };



    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_1.
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoOneSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoOneSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };

    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_3
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoThreeSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoThreeSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };

    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_6
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoSixSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoSixSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };

    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_7
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoSevenSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoSevenSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };


    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_8
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoEightSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoEightSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };


    ///<remarks>
    ///  SafeHandle that wraps a pointer to a PRINTER_INFO_9
    ///</remarks>
    ///<SecurityNote>
    /// Critical    - Base class is SecurityCritical
    ///               The implemention of ReleaseHandle is critical and publicly accessible via Dispose
    ///               ReleaseHandle cannot be protected by a demand because demands cannot be satisfied when finalizing.
    ///</SecurityNote>
    [SecurityCritical]
    private ref class  PrinterInfoNineSafeMemoryHandle sealed : public SafeMemoryHandle 
    {
        public:
            
        ///<SecurityNote>
        /// Critical    - Calls critical constructor to set SafeMemoryHandle's handle.
        /// TreatAsSafe - Handle is set to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoNineSafeMemoryHandle(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;
    };


    private ref class  PropertyCollectionMemorySafeHandle sealed : public System::Runtime::InteropServices::SafeHandle 
    {
        public:

        ///<SecurityNote>
        /// Critical    - Calls critical PropertyCollectionMemorySafeHandle ctor
        /// TreatAsSafe - Calls ctor with trusted handle allocated inside the method
        ///             - Does not expose trusted handle
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        PropertyCollectionMemorySafeHandle^
        AllocPropertyCollectionMemorySafeHandle(
            UInt32   propertyCount
            );

        property
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
        /// Critical    - Calls critical method to free arbitrary HGLOBAL
        ///</SecurityNote>
        [SecurityCritical]
        Boolean 
        virtual ReleaseHandle(
            void
            ) override;        

        ///<SecurityNote>
        /// Critical    - Calls critical AttributeValueInteropHandler::SetValue with critical handle
        ///</SecurityNote>
        [SecurityCritical]
        void
        SetValue(
            String^         propertyName,
            UInt32          index,
            System::Type^   value
            );

        ///<SecurityNote>
        /// Critical    - Calls critical AttributeValueInteropHandler::SetValue with critical handle
        ///</SecurityNote>
        [SecurityCritical]
        void
        SetValue(
            String^     propertyName,
            UInt32      index,
            Object^     value
            );

        private:

        ///<SecurityNote>
        /// Critical    - Sets critical member to arbitrary value
        ///</SecurityNote>
        [SecurityCritical]
        PropertyCollectionMemorySafeHandle(
            IntPtr   Win32Pointer
            );

    };

    private ref class  DocEventFilter
    {
        public:

        ///<SecurityNote>
        /// Critical        : Acceses type from non-APTCA Reachframework.dll (XpsDocumentEventType)
        /// TreatAsSafe     : Type is safe enum
        ///</SecurityNote>
        [SecuritySafeCritical]
        DocEventFilter(
            void
            );

        ///<SecurityNote>
        /// Critical        : Acceses type from non-APTCA Reachframework.dll (XpsDocumentEventType)
        /// TreatAsSafe     : Type is safe enum
        ///</SecurityNote>
        [SecuritySafeCritical]
        Boolean
        IsXpsDocumentEventSupported(
            XpsDocumentEventType    escape
            );

        ///<SecurityNote>
        /// Critical        : Acceses type from non-APTCA Reachframework.dll (XpsDocumentEventType)
        /// TreatAsSafe     : Type is safe enum
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        SetUnsupportedXpsDocumentEvent(
            XpsDocumentEventType    escape
            );

        private:

        ///<SecurityNote>
        /// Critical        : Field for type from non-APTCA Reachframework.dll (XpsDocumentEventType)
        ///</SecurityNote>
        [SecurityCritical]
        array<XpsDocumentEventType>^         eventsfilter;
        static
        Int32                                supportedEventsCount = Int32(XpsDocumentEventType::AddFixedDocumentSequencePost) + 1;
    };

    private ref class PrinterThunkHandler  sealed : public PrinterThunkHandlerBase 
    {
        public:

        ///<SecurityNote>
        /// Critical    - stores a win32 print handle
        /// TreatAsSafe - the win32PrintHandle is created by calling ThunkAddPrinter
        ///               ThunkAddPrinter is only enabled in FullTrust and demands AllPrinting permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        PrinterThunkHandler(
            IntPtr              win32PrintHandle
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkOpenPrinter which in turn calls UnsafeNativeMethods::InvokeOpenPrinter 
        /// TreatAsSafe - this opens a printer handle without requesting Admin privileges
        ///               the class demands DefaultPrinting. Not enabled in Partial Trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        PrinterThunkHandler(
            String^             printName
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkOpenPrinter which in turn calls UnsafeNativeMethods::InvokeOpenPrinter 
        ///               which has SUC applied
        /// TreatAsSafe - if PrinterDefaults.DesiredAccess is PrinterFullAccess,
        ///               AdministratePrinter or AdministrateServer, then the caller must 
        ///               demand AllPrinting, otherwise this class demands DefaultPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        PrinterThunkHandler(
            String^             printName,
            PrinterDefaults^    printerDefaults
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
        /// Critical    - Creates a PrinterThunkHandler object with same desired access as current object
        ///               It ends up calling UnsafeNativeMethods::InvokeOpenPrinter which has SUC applied
        /// TreatAsSafe - if PrinterDefaults.DesiredAccess is PrinterFullAccess,
        ///               AdministratePrinter or AdministrateServer, then the code 
        ///               demands AllPrinting, otherwise, it implicitly demands DefaultPrinting
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        PrinterThunkHandler^
        DuplicateHandler(
            void
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinter which has SUC applied
        /// TreatAsSafe - This class demands DefaultPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkGetPrinter(
            UInt32              level
            );

        ////<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterDriver which has SUC applied
        /// TreatAsSafe - This class demands DefaultPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkGetDriver(
            UInt32              level,
            String^             environment
            );

        ////<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::EnumPrinterDrivers which has SUC applied
        /// TreatAsSafe - This class demands DefaultPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkEnumDrivers(
            UInt32              level,
            String^             environment
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetJob which has SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkGetJob(
            UInt32              level,
            UInt32              jobID
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeEnumJobs which has SUC applied 
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkEnumJobs(
            UInt32              level,
            UInt32              firstJob,
            UInt32              numberOfJobs
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeSetJob which has SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkSetJob(
            UInt32              jobID,
            UInt32              command
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeStartDocPrinter which has SUC applied 
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
        /// Critical    - calls UnsafeNativeMethods::InvokeEndDocPrinter which has SUC applied 
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
        /// Critical    - calls UnsafeNativeMethods::InvokeEndDocPrinter which has SUC applied 
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkStartPagePrinter(
            void
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeEndDocPrinter which has SUC applied 
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkEndPagePrinter(
            void
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::ThunkAbortPrinter which has SUC applied 
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
        /// Critical    - calls UnsafeNativeMethods::InvokeGetSpoolFileHandle which has SUC applied 
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
        /// Critical    - calls UnsafeNativeMethods::InvokeCommitSpoolData which has SUC applied 
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
        /// Critical    - calls UnsafeNativeMethods::InvokeCloseSpoolFileHandle which has SUC applied 
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
            virtual Stream^ get() override;
        }  

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDocumentEvent which has SUC applied 
        /// TreatAsSafe - this method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkDocumentEvent(
            XpsDocumentEventType    escape,
            UInt32                  inBufferSize,
            SafeHandle^             inBuffer,
            UInt32                  outBufferSize,
            SafeMemoryHandle^       outBuffer
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDocumentEvent which has SUC applied 
        /// TreatAsSafe - this method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkDocumentEvent(
            XpsDocumentEventType    escape
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDocumentEvent which has SUC applied 
        /// TreatAsSafe - this method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkDocumentEvent(
            XpsDocumentEventType    escape,
            SafeHandle^             inputBufferSafeHandle
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDocumentEvent which has SUC applied 
        /// TreatAsSafe - this method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        PrinterThunkHandler::
        ThunkDocumentEventPrintTicket(
            XpsDocumentEventType                escapePre,
            XpsDocumentEventType                escapePost,
            SafeHandle^                         inputBufferSafeHandle,
            System::IO::MemoryStream^%          printTicketStream
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDocumentEvent which has SUC applied 
        /// TreatAsSafe - this method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkDocumentEventPrintTicketPost(
            XpsDocumentEventType    escape,            
            SafeMemoryHandle^       xpsDocEventOutputBuffer,
            UInt32                  xpsDocEventOutputBufferSize
            );

        ///<SecurityNote>
        /// Critical    - Calls Critical Dispose on critical SafeHandle type
        ///</SecurityNote>
        [SecurityCritical]
        Boolean
        IsXpsDocumentEventSupported(
            XpsDocumentEventType    escape,
            Boolean                 reset
            );

        ///<SecurityNote>
        /// Critical    - References type from non-APTCA ReachFramework.dll (XpsDocumentEventType)
        /// TreatAsSafe - Type is enum
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        SetUnsupportedXpsDocumentEvent(
            XpsDocumentEventType    escape
            );

        #ifdef XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeAddJob which has SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        IPrinterInfo^
        ThunkAddJob(
            UInt32              level
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeScheduleJob which has SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkScheduleJob(
            UInt32              jobID
            );

        #endif // XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeReportJobProgress which has SUC applied
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

        

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDeletePrinter with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkDeletePrinter(
            void
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkSetPrinterDataStringInternal which in turn calls 
        ///                UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        Boolean
        ThunkSetPrinterDataString(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical - ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        Boolean
        ThunkSetPrinterDataInt32(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical - ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        Boolean
        ThunkSetPrinterDataBoolean(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical - ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        Boolean
        ThunkSetPrinterDataServerEventLogging(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical - ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting 
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        Boolean
        ThunkSetPrinterDataThreadPriority(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical    - ends up calling UnsafeNativeMethods::InvokeGetPrinterData with SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Object^
        ThunkGetPrinterDataString(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkGetPrinterDataStringInternal which in turn calls 
        ///               UnsafeNativeMethods::InvokeGetPrinterData with SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Object^
        ThunkGetPrinterDataInt32(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkGetPrinterDataBooleanInternal which in turn calls
        ///               UnsafeNativeMethods::InvokeGetPrinterData with SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Object^
        ThunkGetPrinterDataBoolean(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkGetPrinterDataThreadPriorityInternal which in turn calls
        ///               UnsafeNativeMethods::InvokeGetPrinterData with SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Object^
        ThunkGetPrinterDataThreadPriority(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls ThunkGetPrinterDataServerEventLoggingInternal which in turn calls
        ///               UnsafeNativeMethods::InvokeGetPrinterData with SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Object^
        ThunkGetPrinterDataServerEventLogging(
            PrinterThunkHandler^    printerHandle,
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    -  ends up calling UnsafeNativeMethods::InvokeSetPrinter API
        /// TreatAsSafe -  demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinter(
            UInt32              command
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeSetPrinter which has SUC applied
        ///             - imperatively demands either DedaultPrinting or AllPrinting, depending on the level
        ///               It demands AllPrinting for all levels, except 9 which doesn't require admin privileges
        ///               and it's needed to enable setting a PrintTicket in Intranet Zone
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkSetPrinter(
            UInt32              level,
            SafeMemoryHandle^   win32PrinterInfo
            );

        ///<SecurityNote>
        /// Critical    -  ends up calling UnsafeNativeMethods::InvokeAddPrinter API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        PrinterThunkHandler^
        ThunkAddPrinter(
            String^             serverName,
            String^             printerName,
            String^             driverName,
            String^             portName,
            String^             printProcessorName,
            String^             comment,
            String^             location,
            String^             shareName,
            String^             separatorFile,
            Int32               attributes,
            Int32               priority,
            Int32               defaultPriority
            );

        ///<SecurityNote>
        /// Critical    -  ends up calling UnsafeNativeMethods::InvokeAddPrinter API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        static
        PrinterThunkHandler^
        ThunkAddPrinter(
            String^                                              serverName,
            DirectInteropForPrintQueue::PrinterInfoTwoSetter^    printInfoTwoLeveThunk
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeEnumPrinters which has SUC applied
        /// TreatAsSafe - this class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        IPrinterInfo^
        ThunkEnumPrinters(
            String^             serverName,
            UInt32              level,
            UInt32              flags
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeAddPrinterConnection which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Boolean
        ThunkAddPrinterConnection(
            String^             path
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeDeletePrinterConnection  which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        [SecuritySafeCritical]
        static
        Boolean
        ThunkDeletePrinterConnection(
            String^             path
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetDefaultPrinter  which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.        
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        String^
        ThunkGetDefaultPrinter(
            void
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeSetDefaultPrinter  which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Boolean
        ThunkSetDefaultPrinter(
            String^         path
            );

        ///<SecurityNote>
        /// Critical    - calls unmanaged UnsafeNativeMethods::GetComputerName API
        /// TreatAsSafe - innocuous call. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        String^
        GetLocalMachineName(
            void
            );

        #ifdef XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeWritePrinter  which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Int32
        ThunkWritePrinter(
            PrinterThunkHandler^    printerHandle,
            array<Byte>^            array,
            Int32                   offset,
            Int32                   count,
            Int32&                  writtenDataCount
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeFlushPrinter  which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        Int32
        ThunkFlushPrinter(
            PrinterThunkHandler^    printerHandle,
            array<Byte>^            array,
            Int32                   offset,
            Int32                   count,
            Int32&                  flushedByteCount,
            Int32                   portIdleTime
            );

        #endif //XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls unmamanged GetPrinterDriver and needs to suppress unmanaged code
        /// TreatAsSafe - non-admin operation. This method demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkIsMetroDriverEnabled(
            void
            );

        private:

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetSpoolFileHandle which has SUC applied 
        ///</SecurityNote>
        [SecurityCritical]
        FileStream^ 
        PrinterThunkHandler::
        CreateSpoolStream(
            IntPtr fileHandle
            );

        /// <SecurityNote>
        ///     Critical    -   Calls critical PrinterInfo* constructors
        ///     TreatAsSafe -   Demands DefaultPrinting permissions
        /// </SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        IPrinterInfo^
        GetManagedPrinterInfoObject(
            UInt32              level,
            SafeMemoryHandle^   win32HeapBuffer,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - Signature has critical type (SafeMemoryHandle)
        /// TreatAsSafe - Does not reference critical parameter
        [SecuritySafeCritical]
        static
        IPrinterInfo^
        GetManagedDriverInfoObject(
            UInt32              level,
            SafeMemoryHandle^   win32HeapBuffer,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - Calls Critical JobInfo constructors
        /// TreatAsSafe - Demands DefaultPrinting
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        static
        IPrinterInfo^
        GetManagedJobInfoObject(
            UInt32              level,
            SafeMemoryHandle^   win32HeapBuffer,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeOpenPrinter which has SUC applied
        /// TreatAsSafe - if PrinterDefaults.DesiredAccess is PrinterFullAccess,
        ///               AdministratePrinter or AdministrateServer, then the caller must 
        ///               demand AllPrinting, otherwise, this class demands DefaultPrinting
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Boolean
        ThunkOpenPrinter(
            String^             name,
            PrinterDefaults^    openPrinterDefaults
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeClosePrinter which has SUC applied
        /// TreatAsSafe - innocuous call which closes a printer handle
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Runtime::ConstrainedExecution::ReliabilityContract(System::Runtime::ConstrainedExecution::Consistency::WillNotCorruptState,
                                                           System::Runtime::ConstrainedExecution::Cer::Success)]
        Boolean
        ThunkClosePrinter(
            void
            );

        ///<SecurityNote>
        /// Critical -  ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinterDataStringInternal(
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical -  ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinterDataInt32Internal(
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical -  ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinterDataBooleanInternal(
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical -  ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinterDataServerEventLoggingInternal(
            String^                 valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Critical -  ends up calling UnsafeNativeMethods::InvokeSetPrinterData API with SUC applied
        /// TreatAsSafe - demands PrintingPermissions::AllPrinting
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand, 
         Level = System::Drawing::Printing::PrintingPermissionLevel::AllPrinting)]
        Boolean
        ThunkSetPrinterDataThreadPriorityInternal(
            String^                 valueName,
            Object^                 value
            );

        #ifdef XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeWritePrinter which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkWritePrinterInternal(
            array<Byte>^            array,
            Int32                   offset,
            Int32                   count,
            Int32&                  writtenDataCount
            );  

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeFlushPrinter which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Int32
        ThunkFlushPrinterInternal(
            array<Byte>^            array,
            Int32                   offset,
            Int32                   count,
            Int32&                  flushedByteCount,
            Int32                   portIdleTime
            );

        #endif //XPSJOBNOTIFY

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterData which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Object^
        ThunkGetPrinterDataStringInternal(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterData which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Object^
        ThunkGetPrinterDataInt32Internal(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterData which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Object^
        ThunkGetPrinterDataBooleanInternal(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterData which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Object^
        ThunkGetPrinterDataThreadPriorityInternal(
            String^                 valueName
            );
        
        ///<SecurityNote>
        /// Critical    - calls UnsafeNativeMethods::InvokeGetPrinterData which has SUC applied
        /// TreatAsSafe - non-admin operation. This class demands DefaultPrinting.
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        Object^
        ThunkGetPrinterDataServerEventLoggingInternal(
            String^                 valueName
            );
        
        ///<SecurityNote>
        /// Critical    - Calls critical base constructor
        /// TreatAsSafe - Calls constructor with safe values
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterThunkHandler(
            void
            );

        ///<SecurityNote>
        /// Critical    - Used to open a handle to a printer device
        ///</SecurityNote>
        [SecurityCritical]
        String^                             printerName;

        PrinterDefaults^                    printerDefaults;
        UInt32                              printersCount;
        Boolean                             isDisposed;
        Boolean                             isRunningDownLevel;

        ///<SecurityNote>
        /// Critical    - Stream used to send raw data to printer device
        ///</SecurityNote>
        [SecurityCritical]
        FileStream^                         spoolStream;

        ///<SecurityNote>
        /// Critical    - Used to make trust decisions about what critical API's are allowed to be called.
        ///</SecurityNote>
        [SecurityCritical]
        Boolean                             isInPartialTrust;

        int                                 jobIdentifier;

        ///<SecurityNote>
        /// Critical        : Field for type from non-APTCA Reachframework.dll (XpsDocumentEventType)
        ///</SecurityNote>
        [SecurityCritical]
        XpsDocumentEventType                previousXpsDocEventEscape;
        
        DocEventFilter^                     docEventFilter;
        static
        const 
        int                                 MaxPath = MAX_PATH;
    };

    private ref class SecurityHelper
    {
        public:

        ///<SecurityNote>
        /// Critical    - Contains a security demand
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        DemandAllPrinting(
            void
            );  

        ///<SecurityNote>
        /// Critical    - Contains a security demand
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        DemandDefaultPrinting(
            void
            );

        ///<SecurityNote>
        /// Critical    - Contains a security demand
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        DemandPrintingPermissions(
            PrintSystemDesiredAccess    desiredAccess
            );

        private:

        SecurityHelper(
            void
            ) 
        {
        }

    };

}
}
}
#endif

