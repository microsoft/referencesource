#pragma once

#ifndef __INTEROPPRINTERINFOUNMANAGEDBUILDER_HPP__
#define __INTEROPPRINTERINFOUNMANAGEDBUILDER_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                

        InteropPrinterInfoUnmanagedBuilder.hpp
        
    Abstract:

        Utility classes that allocate and free the unmanaged printer info
        buffers that are going to be sent to the Win32 APIs.
        
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
    namespace Win32ApiThunk
    {
        ref class UnmanagedPrinterInfoLevelBuilder abstract
        {
            public:

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoOne(
                void
                );        

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoOne(
                IntPtr                  win32PrinterInfoOne
                );

            
            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoTwo(
                void
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            ///               Writes arbitraty data to allocated native memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            IntPtr
            BuildUnmanagedPrinterInfoTwo(
                String^                 serverName,
                String^                 printerName,
                String^                 driverName,
                String^                 portName,
                String^                 printProcessorName,
                String^                 comment,
                String^                 location,
                String^                 shareName, 
                String^                 separatorFile,
                Int32                   attributes,        
                Int32                   priority,
                Int32                   defaultPriority
                );

            ///<SecurityNote>
            /// Critical    : Writes to arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            IntPtr
            WriteStringInUnmanagedPrinterInfo(
                IntPtr                  win32PrinterInfo,
                String^                 stringValue,
                int                     offset
                );

            ///<SecurityNote>
            /// Critical    : Writes to arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            bool
            WriteIntPtrInUnmanagedPrinterInfo(
                IntPtr                  win32PrinterInfo,
                IntPtr                  pointerValue,
                int                     offset
                );

            ///<SecurityNote>
            /// Critical    : Writes to arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            bool
            WriteInt32InUnmanagedPrinterInfo(
                IntPtr                  win32PrinterInfo,
                Int32                   value,
                int                     offset
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoTwo(
                IntPtr                  win32PrinterInfoTwo
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoThree(
                void
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoThree(
                IntPtr                  win32PrinterInfoThree
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoSix(
                void
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoSix(
                IntPtr                  win32PrinterInfoSix
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoSeven(
                void
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoSeven(
                IntPtr                  win32PrinterInfoSeven
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoEight(
                void
                );

            ///<SecurityNote>
            /// Critical    : Writes to arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            bool
            WriteDevModeInUnmanagedPrinterInfoEight(
                IntPtr                  win32PrinterInfoEight,
                IntPtr                  pDevMode
                );

            ///<SecurityNote>
            /// Critical    : Writes to arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            bool
            WriteDevModeInUnmanagedPrinterInfoNine(
                IntPtr                  win32PrinterInfoNine,
                IntPtr                  pDevMode
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoEight(
                IntPtr                  win32PrinterInfoEight 
                );

            ///<SecurityNote>
            /// Critical    : Calls critical Marshal.AllocHGlobal
            /// TreatAsSafe : Call is made with safe arguments
            ///</SecurityNote>
            [SecuritySafeCritical]
            static
            IntPtr
            BuildEmptyUnmanagedPrinterInfoNine(
                void
                );

            ///<SecurityNote>
            /// Critical    : Frees arbitrary memory
            ///</SecurityNote>
            [SecurityCritical]
            static
            void
            FreeUnmanagedPrinterInfoNine(
                IntPtr                  win32PrinterInfoNine 
                );
        };

        private ref class  UnmanagedXpsDocEventBuilder abstract
        { 
            public:

            ///<SecurityNote>
            /// Critical    : Returns critical data
            ///</SecurityNote>
            [SecurityCritical]
            static
            SafeHandle^
            XpsDocEventFixedDocSequence(
                XpsDocumentEventType    escape,
                UInt32                  jobId,
                String^                 jobName,
                System::IO::Stream^     printTicket,
                Boolean                 mustAddPrintTicket
                );
            
            ///<SecurityNote>
            /// Critical    : Returns critical data
            ///</SecurityNote>
            [SecurityCritical]
            static
            SafeHandle^
            XpsDocEventFixedDocument(
                XpsDocumentEventType    escape,
                UInt32                  fixedDocumentNumber,
                System::IO::Stream^     printTicket,
                Boolean                 mustAddPrintTicket
                );

            ///<SecurityNote>
            /// Critical    : Returns critical data
            ///</SecurityNote>
            [SecurityCritical]
            static
            SafeHandle^
            XpsDocEventFixedPage(
                XpsDocumentEventType    escape,
                UInt32                  fixedPageNumber,
                System::IO::Stream^     printTicket,
                Boolean                 mustAddPrintTicket
                );
            
            
        };
    }
}
}
}
#endif
