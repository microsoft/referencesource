#pragma once

#ifndef __INTEROPJOBINFO_HPP__
#define __INTEROPJOBINFO_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropJobInfo.hpp
        
    Abstract:

        The file contains the definition for the managed classes that 
        hold the pointers to the JOB_INFO_ unmanaged structures and 
        know how to retrieve a property based on it's name. 
        
    Author: 

        Adina Trufinescu (adinatru) October 13, 2003
                                                                             
    Revision History:  
--*/
namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
namespace DirectInteropForJob
{
    using namespace System::Security;
    using namespace System::Security::Permissions;
    using namespace System::Drawing::Printing;

    private ref class JobInfoOne  : public IPrinterInfo
    {
	    public :

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        JobInfoOne(
		    SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose to free critical SafeMemoryHandle.
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
            String^         name
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical data (e.g. network name of printer the job is scheuled on)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^         name,
            UInt32          index
            );

        virtual bool
        SetValueFromName(
            String^         name,
            Object^         value
            );

        property
        virtual
        UInt32
        Count
        {
            UInt32 get();
        }

        private:

        static
        JobInfoOne(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical methods
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
        GetJobId(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetServerName(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPrinterName(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetUserName(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetDocumentName(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetDatatype(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetStatusString(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetStatus(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPriority(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPosition(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetTotalPages(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPagesPrinted(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetTimeSubmitted(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            JOB_INFO_1W*        unmanagedPrinterInfo
            );

        static
        Hashtable^                  getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           jobInfoOneSafeHandle;

        bool                        isDisposed;
        UInt32                      jobsCount;

    };


    private ref class JobInfoTwo  : public IPrinterInfo
    {
	    public :

        ///<SecurityNote>
        /// Critical    - Sets critical member.
        ///</SecurityNote>
        [SecurityCritical]
        JobInfoTwo(
		    SafeMemoryHandle^       unmanagedPrinterInfo,
            UInt32                  count
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Dispose to free critical SafeMemoryHandle.
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
            String^         name
            );

        ///<SecurityNote>
        /// Critical     - Calls critical Win32SafeHandle
        ///              - Performs unsafe reinterpret_cast on IntPtr from Win32SafeHandle  
        ///              - Returns critical data (e.g. network name of printer the job is scheuled on)
        /// TreatAsSafe  - Demands printing permission
        ///</SecurityNote>
        [SecuritySafeCritical]
        [PrintingPermission(SecurityAction::Demand, Level=PrintingPermissionLevel::DefaultPrinting)]
        virtual Object^
        GetValueFromName(
            String^         name,
            UInt32          index
            );

        virtual bool
        SetValueFromName(
            String^         name,
            Object^         value
            );

        property
        virtual
        UInt32
        Count
        {
            UInt32 get();
        }

        private:
        
        static
        JobInfoTwo(
            void
            )
        {
            getAttributeMap = gcnew Hashtable();

            RegisterAttributeMaps();
        }

        /// <SecurityNote>
        ///     Critical    : Accesses critical methods
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
        GetJobId(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetServerName(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPrinterName(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetUserName(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetDocumentName(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetDatatype(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetStatusString(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetStatus(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPriority(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPosition(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetTotalPages(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetPagesPrinted(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetTimeSubmitted(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static        
        Object^
        GetSecurityDescriptor(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetNotifyName(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetQueueDriverName(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetPrintProcessor(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetPrintProcessorParameters(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetStartTime(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetUntilTime(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetTimeSinceSubmitted(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetSize(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetDevMode(
            JOB_INFO_2W* unmanagedJobInfo
            );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        delegate
        Object^
        GetValue(
            JOB_INFO_2W*        unmanagedPrinterInfo
            );


        static
        Hashtable^                  getAttributeMap;

        ///<SecurityNote>
        /// Critical    - SafeHandle that wraps native memory.
        ///</SecurityNote>
        [SecurityCritical]
        SafeMemoryHandle^           jobInfoTwoSafeHandle;

        bool                        isDisposed;
        UInt32                      jobsCount;

    };

}
}
}
}
#endif
