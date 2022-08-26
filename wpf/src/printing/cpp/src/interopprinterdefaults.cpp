/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:    

        InteropPrinterDefaults.cpp
        
    Abstract:

        PrinterDefaults class is the managed class corresponding 
        to PRINTER_DEFAULTS Win32 structure. It has the same
        memory layout as PRINTER_DEFAULTS structure.
        
    Author: 

        Adina Trufinescu April 24, 2003
                                                                             
    Revision History:  
--*/

#include "win32inc.hpp"

#ifndef  __INTEROPNAMESPACEUSAGE_HPP__
#include <InteropNamespaceUsage.hpp>
#endif

#ifndef  __PRINTSYSTEMSECURITY_HPP__
#include <PrintSystemSecurity.hpp>
#endif

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif



using namespace MS::Internal;


    /*++

    Routine Name:   

        PrinterDefaults

    Routine Description:

        Constructor 

    Arguments:

        dataType      -   printing datatype. RAW by default
        devmode       -   managed devmode class used for initialization
        desiredAccess -   desired access for the printer
        
    Return Value:

        N\A

    --*/
    PrinterDefaults::
    PrinterDefaults(
        String^                       dataType,
        PrintWin32Thunk::DeviceMode^  devMode,
        PrintSystemDesiredAccess      desiredAccess
        ) : defaultDataType(dataType),
            defaultDeviceMode(NULL),
            defaultDesiredAccess(desiredAccess)
    {
        if (devMode && devMode->Data != nullptr)
        {
            defaultDeviceMode = Marshal::AllocHGlobal(devMode->Data->Length);

            Marshal::Copy((array<Byte>^)devMode->Data, 
                          0, 
                          defaultDeviceMode, 
                          devMode->Data->Length);        

        }
    }

    /*++

    Routine Name:   

        ~PrinterDefaults

    Routine Description:

        Destructor 

    Arguments:

        None
        
    Return Value:

        N\A

    --*/
    PrinterDefaults::
    ~PrinterDefaults(
        void
        )
    {
        InternalDispose(true);
    }

    /*++

    Routine Name:   

        !PrinterDefaults

    Routine Description:

        Finalizer

    Arguments:

        None
        
    Return Value:

        N\A

    --*/
    PrinterDefaults::
    !PrinterDefaults(
        void
        )
    {
        InternalDispose(false);
    }

    void
    PrinterDefaults::
    InternalDispose(
        bool disposing
        )
    {
        if (defaultDeviceMode != IntPtr::Zero)
        {
			SecurityHelper::DemandDefaultPrinting();
            Marshal::FreeHGlobal(defaultDeviceMode);
            defaultDeviceMode = IntPtr::Zero;
        }
    }

    /*++

    Routine Name:   

        get_DesiredAccess

    Routine Description:

        property     

    Arguments:

        None
        
    Return Value:

        N\A

    --*/
    PrintSystemDesiredAccess
    PrinterDefaults::DesiredAccess::
    get(
        void
        )
    {
        return defaultDesiredAccess;
    }
