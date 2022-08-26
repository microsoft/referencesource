#pragma once

#ifndef __INTEROPPRINTERINFO_HPP__
#define __INTEROPPRINTERINFO_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropPrinterInfo.hpp

    Abstract:

        The file contains the definition for the managed classes that 
        hold the pointers to the PRINTER_INFO_ unmanaged structures and know how 
        to retrieve a property based on it's name. 
        
    Author: 

        Adina Trufinescu (AdinaTru) April 24th 2003
                                                                             
    Revision History:  
--*/
namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
    ref class PrinterThunkHandler;
    ref class SafeMemoryHandle;

namespace DirectInteropForPrintQueue
{    
    using namespace System::Security;
    using namespace System::Security::Permissions;
    using namespace System::Drawing::Printing;

    private ref class PrinterInfoOne : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoOne(
            SafeMemoryHandle^   unmanagedPrinterInfo,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Initializes critical member to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoOne(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );
        
        property
        UInt32
        Count
        {
            UInt32 virtual get();
        }

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        Object^
        GetValueFromName(
            String^             valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (e.g. Network name of the printer)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^             valueName,
            UInt32              index
            );

        virtual bool
        SetValueFromName(
            String^             valueName,
            Object^             value
            );

        private:

        // FIX: remove pragma. done to fix compiler error which will be fixed later.
        #pragma warning ( disable:4567 )
        static
        PrinterInfoOne(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical members (unsafe delegates)
        ///     TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetComment(
            PRINTER_INFO_1W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetDescription(
            PRINTER_INFO_1W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetFlags(
            PRINTER_INFO_1W*    unmanagedPrinterInfo
            );
        
        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            PRINTER_INFO_1W*    unmanagedPrinterInfo
            );

        static
        Hashtable^              getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^       printerInfoOneSafeHandle;

        UInt32                  printersCount;
    };


    private ref class PrinterInfoTwoGetter : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoTwoGetter(
            SafeMemoryHandle^   unmanagedPrinterInfo,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }
        
        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }
        
        Object^
        GetValueFromName(
            String^             valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (e.g. default printer DEVMODE)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^             valueName,
            UInt32              index
            );

        virtual bool
        SetValueFromName(
            String^             valueName,
            Object^             value
            );

        private:

        static
        PrinterInfoTwoGetter(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical members (unsafe delegates)
        ///     TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetServerName(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPrinterName(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static 
        Object^
        GetShareName(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static 
        Object^
        GetPortName(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static 
        Object^
        GetDriverName(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetComment(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetLocation(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetDeviceMode(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );
        
        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetSeparatorFile(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^ 
        GetPrintProcessor(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetPrintProcessorDatatype(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^ 
        GetPrintProcessorParameters(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetSecurityDescriptor(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetAttributes(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetPriority(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetDefaultPriority(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetStartTime(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetUntilTime(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );
        
        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetStatus(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetAveragePPM(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetJobs(
            PRINTER_INFO_2W*    unmanagedPrinterInfo
            );
        
        private:

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            PRINTER_INFO_2W*    unmanagedPrinterInfo    
            );

        static
        Hashtable^              getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^       printerInfoTwoSafeHandle;

        UInt32                  printersCount;        
    };


    private ref class PrinterInfoTwoSetter sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Initializes critical member to safe value
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoTwoSetter(
            void
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///             - Calls critical IPrinterInfo.get_Win32SafeHandle().
        /// TreatAsSafe - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        PrinterInfoTwoSetter(
            PrinterThunkHandler^    printerHandler
            );

        ///<SecurityNote>
        /// Critical    - Calls Dispose on critical SafeMemoryHandles
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }
        
        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }
        
        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Changes the default DEVMODE of the printer
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        ///<SecurityNote>
        /// Critical     - References critical methods
        ///  TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );
        
        static
        IntPtr
        SetServerName(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrinterName(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetShareName(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPortName(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetDriverName(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetComment(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
        
        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetLocation(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetSeparatorFile(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrintProcessor(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrintProcessorDatatype(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrintProcessorParameters(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
        
        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetSecurityDescriptor(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetAttributes(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPriority(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetDefaultPriority(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetStartTime(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
        
        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetUntilTime(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        static
        IntPtr
        SetStatus(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
        
        static
        IntPtr
        SetAveragePPM(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        static
        IntPtr
        SetJobs(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
                
        private:

        static
        PrinterInfoTwoSetter(
            void
            )
        {
            setAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        delegate
        IntPtr
        SetValue(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );
        
        static
        Hashtable^                  setAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           win32PrinterInfoSafeHandle;

        ///<SecurityNote>
        /// Critical: Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        array<SafeMemoryHandle^>^   internalMembersList;
        int                         internalMembersIndex;
    };

    private ref class PrinterInfoThree sealed : public IPrinterInfo
    {
        public :

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoThree(
            SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Initializes critical member to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoThree(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        Object^
        GetValueFromName(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (the SECURITY_DESCRIPTOR of the printer)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoThreeSafeHandle;

        UInt32                      printersCount;
    };

    private ref class PrinterInfoFourGetter sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoFourGetter(
            SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }
        
        Object^
        GetValueFromName(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (e.g. the path to the print server for network printers)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        static
        PrinterInfoFourGetter(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical members (unsafe delegates)
        ///     TreatAsSafe : Does not execute delegates
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetAttributes(
            PRINTER_INFO_4W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetServerName(
            PRINTER_INFO_4W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPrinterName(
            PRINTER_INFO_4W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            PRINTER_INFO_4W*        unmanagedPrinterInfo
            );

        static
        Hashtable^                  getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoFourSafeHandle;

        UInt32                      printersCount;

    };

    private ref class PrinterInfoFourSetter sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    :  Instantiates array of critical SafeMemoryHandle
        /// TreatAsSafe :  Array is initialized to safe value (all members are null)
        ///                Does not expose array or array members
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoFourSetter(
            PrinterThunkHandler^        printerThunkHandle 
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandles.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///             - Calls critical IPrinterInfo.get_Win32SafeHandle().
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }
        
        virtual Object^
        GetValueFromName(
            String^                     valueName,
            UInt32                      index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Sets critical printer metadata (e.g. the path to the print server for network printers)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                     valueName,
            Object^                     value
            );

        private:

        static
        PrinterInfoFourSetter(
            void
            )
        {
            setAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        ///<SecurityNote>
        /// Critical     - References critical methods
        /// TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        delegate
        IntPtr
        SetValue(
            IntPtr                      valueName,
            Object^                     value
            );

        static
        IntPtr
        SetServerName(
            IntPtr                      valueName,
            Object^                     value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrinterName(
            IntPtr                      printerInfoTwoBuffer,
            Object^                     value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetAttributes(
            IntPtr                      printerInfoTwoBuffer,
            Object^                     value
            );

        static
        Hashtable^                      setAttributeMap;
        IPrinterInfo^                   printerInfo;
        
        ///<SecurityNote>
        /// Critical: Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        array<SafeMemoryHandle^>^       internalMembersList;
        int                             internalMembersIndex;
    };

    private ref class PrinterInfoFiveGetter sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoFiveGetter(
            SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (e.g. the printer port which is normally LPT1: but could be a file path or ip address)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual
        Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        static
        PrinterInfoFiveGetter(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical members (unsafe delegates)
        ///     TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetAttributes(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetPortName(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPrinterName(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetDeviceNotSelectedTimeout(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetTransmissionRetryTimeout(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            PRINTER_INFO_5W*        unmanagedPrinterInfo
            );
       
        static
        Hashtable^                  getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoFiveSafeHandle;

        UInt32                      printersCount;
    };

    private ref class PrinterInfoFiveSetter sealed  : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    :  Instantiates critical SafeMemoryHandle
        /// TreatAsSafe :  Does not expose handle
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoFiveSetter(
            PrinterThunkHandler^    printThunkHandle
            );

        ///<SecurityNote>
        /// Critical    - Calls critical IPrinterInfo.Release
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///             - Calls critical IPrinterInfo.get_Win32SafeHandle().
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Sets critical printer metadata (e.g. printer port)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        static
        PrinterInfoFiveSetter(
            void
            )
        {
            setAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        ///<SecurityNote>
        /// Critical     - References critical methods
        /// TreatAsSafe  - Does not execute methods (places methods in a collection of delegates)
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        delegate
        IntPtr
        SetValue(
            IntPtr                  unmanagedValue,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPrinterName(
            IntPtr                  valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetPortName(
            IntPtr                  valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetAttributes(
            IntPtr                  valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetTransmissionRetryTimeout(
            IntPtr                  valueName,
            Object^                 value
            );

        ///<SecurityNote>
        /// Calls critical method to write to structure in native memory
        ///</SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        SetDeviceNotSelectedTimeout(
            IntPtr                  printerInfoTwoBuffer,
            Object^                 value
            );

        static
        Hashtable^                  setAttributeMap;
        IPrinterInfo^               printerInfo;
        
        ///<SecurityNote>
        /// Critical: Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        array<SafeMemoryHandle^>^   internalMembersList;
        int                         internalMembersIndex;
    };

    private ref class PrinterInfoSix sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoSix(
            SafeMemoryHandle^           unmanagedPrinterInfo,
            UInt32                      count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Initializes critical member to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoSix(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        Object^
        GetValueFromName(
            String^                     valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer data (e.g. Printer status - busy, idle, paper jam)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^                     valueName,
            UInt32                      index
            );

        virtual bool
        SetValueFromName(
            String^                     valueName,
            Object^                     value
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        private:

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^               printerInfoSixSafeHandle;

        UInt32                          printersCount;
    };

    private ref class PrinterInfoSeven sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoSeven(
            SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Initializes critical member to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoSeven(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        virtual
        Object^
        GetValueFromName(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns potentially critical data from printer device (directory services GUID of the printer)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Sets critical metadata for printer devices (directory services GUID of the printer)  
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        static
        PrinterInfoSeven(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();
            setAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();            
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical members (unsafe delegates)
        ///     TreatAsSafe : Does not execute methods (places methods in a collection of delegates)
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        void
        RegisterAttributeMaps(
            void
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetObjectGUID(
            PRINTER_INFO_7W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetAction(
            PRINTER_INFO_7W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            PRINTER_INFO_7W*        unmanagedPrinterInfo
            );
       
        delegate
        bool
        SetValue(
            IntPtr                  printerInfoSevenBuffer,
            Object^                 value
            );

        static
        bool
        SetObjectGUID(
            IntPtr                  printerInfoSevenBuffer,
            Object^                 value
            );

        static
        bool
        SetAction(
            IntPtr                  printerInfoSevenBuffer,
            Object^                 value
            );
        

        static
        Hashtable^                  getAttributeMap;

        static
        Hashtable^                  setAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoSevenSafeHandle;

        bool                        objectOwnsInternalUnmanagedMembers;
        UInt32                      printersCount;

    };

    private ref class PrinterInfoEight sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoEight(
            SafeMemoryHandle^   unmanagedPrinterInfo,
            UInt32              count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Sets critical memeber to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoEight(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        virtual
        Object^
        GetValueFromName(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer metadata (Per computer default printer settings)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual
        Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Sets critical printer metadata (Per computer default printer settings)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoEightSafeHandle;

        bool                        objectOwnsInternalUnmanagedMembers;
        UInt32                      printersCount;

    };

    private ref class PrinterInfoNine sealed : public IPrinterInfo
    {
        public:

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        PrinterInfoNine(
            SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        /// TreatAsSafe - Sets critical member to safe value.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterInfoNine(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose on critical SafeMemoryHandle.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Release(
            void
            );

        property
        UInt32
        Count
        {
            virtual UInt32 get();
        }

        property
        SafeMemoryHandle^
        Win32SafeHandle
        {
            ///<SecurityNote>
            /// Critical    - Returns critical SafeMemoryHandle.
            ///</SecurityNote>
            [SecurityCritical]
            virtual SafeMemoryHandle^ get();
        }

        Object^
        GetValueFromName(
            String^                 valueName
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical printer metadata (Per user default printer settings)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^                 valueName,
            UInt32                  index
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Calls critical SafeMemoryHandle ctor
        ///              - Sets critical printer metadata (Per user default printer settings)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual bool
        SetValueFromName(
            String^                 valueName,
            Object^                 value
            );

        private:

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           printerInfoNineSafeHandle;

        bool                        objectOwnsInternalUnmanagedMembers;
        UInt32                      printersCount;

    };
}
}
}
}
#endif  
