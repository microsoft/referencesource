/*++
                                                                              
    Copyright (C) 2004 - 2005 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        InteropDocInfo.cpp
        
    Abstract:

        This file contains the definitions of the managed classes corresponding 
        to DOC_INFO_X Win32 structures. DocInfoX class has the same
        memory layout as DOC_INFO_X structure.
        
    Author: 

        Ali Naqvi (alinaqvi)    Microsoft 30 2005
                                                                             
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

DocInfoThree::
DocInfoThree(
    String^     name,
    String^     outputFile,
    String^     dataType,
    Int32       flags
    ) : docName((name != nullptr) ? name : String::Empty),
        docOutputFile(outputFile),
        docDataType(dataType),
        docFlags(flags)
{
}
