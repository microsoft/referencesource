#pragma once

#ifndef __INTEROPPRINTERDEFAULTS_HPP__
#define __INTEROPPRINTERDEFAULTS_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropPrinterDefaults.hpp
        
    Abstract:

        This file contains the definition of the managed class corresponding 
        to PRINTER_DEFAULTS Win32 structure. PrinterDefaults class has the same
        memory layout as PRINTER_DEFAULTS structure.
        
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
    [StructLayout(LayoutKind::Sequential, CharSet=CharSet::Unicode)]
    private ref class  PrinterDefaults : 
	public IDisposable
    { 
        public:

        ///<SecurityNote>
        /// Critical    : Calls critical Marshal::AllocHGlobal and Marshal::Copy to clone devMode parameter
        /// TreatAsSafe : Use of AllocHGlobal and Copy are safe
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrinterDefaults(
            String^                                     dataType,
            DeviceMode^                                 devMode,
            System::Printing::PrintSystemDesiredAccess  desiredAccess
            );

        ~PrinterDefaults(
            void
            );

        property
        System::Printing::PrintSystemDesiredAccess
        DesiredAccess
        {
            System::Printing::PrintSystemDesiredAccess get();
        }

        protected:

        !PrinterDefaults(
            void
            );

        ///<SecurityNote>
        /// Critical    : Calls critical Marshal::FreeHGlobal to free native memory
		/// Safe        : Demands printing permissions
        ///</SecurityNote>        
        [SecuritySafeCritical]
        virtual
        void
        InternalDispose(
            bool    disposing
            );
        
        private:

        [MarshalAs(UnmanagedType::LPWStr)]
        String^             defaultDataType;

        [MarshalAs(UnmanagedType::SysInt)]
        IntPtr              defaultDeviceMode; 

        [MarshalAs(UnmanagedType::U4)]
        System::Printing::PrintSystemDesiredAccess defaultDesiredAccess; 
    };
}
}
}
#endif
