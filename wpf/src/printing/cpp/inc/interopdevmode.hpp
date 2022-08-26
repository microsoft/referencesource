#ifndef __INTEROPDEVMODE_HPP__
#define __INTEROPDEVMODE_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name: 

        InteropDevMode.cpp
        
    Abstract:

        The file contains the definition for the managed classe that 
        wraps a DEVMODE Win32 structure and expose it as a byte array
        to the managed code.
        
    Author: 

        Adina Trufinescu (AdinaTru) April 24th 2003
                                                                             
    Revision History:  
--*/

#pragma once
namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
    using namespace System::Security;

    [StructLayout(LayoutKind::Sequential, CharSet=CharSet::Auto)]
    private ref class DeviceMode sealed
    {
        public:

        DeviceMode(
		    array<Byte>^    devMode
		    );

        /// <SecurityNote>
        ///     Critical : Unverifiable pointer access
        /// </SecurityNote>
        [SecurityCritical]
        DeviceMode(
		    void*       devModeUnmanaged
		    );

        property
	    array<Byte>^
	    Data
        {
            array<Byte>^ get();
        }

        private:

	    array<Byte>^    data; 
	    UInt32	size;
    };    

}
}
}
#endif
