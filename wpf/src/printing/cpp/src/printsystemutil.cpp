/*++
                                                                              
    Copyright (C) 2004 - 2005 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                
        PrintSystemUtil.cpp                                                             
                                                                              
    Abstract:
        Utility help classes.
        
    Author:                                                                     
        Ali Naqvi   (alinaqvi)     Feb 4, 2005                            
                                                                             
    Revision History:                                                           
--*/
#include "win32inc.hpp"

using namespace System;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Reflection;

#ifndef  __PRINTSYSTEMUTIL_HPP__
#include <PrintSystemUtil.hpp>
#endif



using namespace System::Printing;


/*-----------------------------------------------------------------------------------------------
                        InternalExceptionResourceManager Implementation
-------------------------------------------------------------------------------------------------*/
InternalExceptionResourceManager::
InternalExceptionResourceManager(
    void
    ) : ResourceManager("System.Printing",
                        Assembly::GetExecutingAssembly())
{
}
