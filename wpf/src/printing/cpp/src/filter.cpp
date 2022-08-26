/*++

    Copyright (C) 2002 - 2003 Microsoft Corporation
    All rights reserved.

    Module Name:
        Filter.cpp

    Abstract:

    Author:
        Khaled Sedky (khaledS) 16-December-2002

    Revision History:
--*/
#include "win32inc.hpp"

using namespace System;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Xml;
using namespace System::Xml::XPath;
using namespace System::Collections::Specialized;

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif

#ifndef  __PRITNSYSTEMINC_HPP__
#include <PrintSystemInc.hpp>
#endif



using namespace System::Printing;

PrintFilter::
PrintFilter(
    String^  filterName
    )
{
}

void
PrintFilter::
InternalDispose(
    bool    disposing
    )
{
    if(!this->IsDisposed)
    {
        System::Threading::Monitor::Enter(this);
        {
            __try
            {
                PrintSystemObject::InternalDispose(disposing);
            }
            __finally
            {
                this->IsDisposed = true;
                System::Threading::Monitor::Exit(this);
            }
        }
    }
}
