/*++

    Copyright (C) 2002 - 2003 Microsoft Corporation
    All rights reserved.

    Module Name:
        PrintSystemNotifications.cpp

    Abstract:

    Author:
        Adina Trufinescu (AdinaTru) 31-December-2002

    Revision History:
        Khaled Sedky (khaledS) 10-December-2003
--*/
#include "win32inc.hpp"

using namespace System;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Specialized;

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif

#ifndef  __PRINTSYSTEMINC_HPP__
#include <PrintSystemInc.hpp>
#endif

#ifndef  __PRINTSYSTEMPATHRESOLVER_HPP__
#include <PrintSystemPathResolver.hpp>
#endif

using namespace System::Printing;

#ifndef  __PRINTSYSTEMPATHRESOLVER_HPP__
#include <PrintSystemPathResolver.hpp>
#endif


PrintSystemObjectPropertyChangedEventArgs::
PrintSystemObjectPropertyChangedEventArgs(
    String^ eventName
    ) : propertyName(eventName)
{
}

PrintSystemObjectPropertyChangedEventArgs::
~PrintSystemObjectPropertyChangedEventArgs(
    void
    )
{
}

String^
PrintSystemObjectPropertyChangedEventArgs::PropertyName::
get(
    void
    )
{
    return propertyName;
}

PrintSystemObjectPropertiesChangedEventArgs::
PrintSystemObjectPropertiesChangedEventArgs(
    StringCollection^   events
    ) : propertiesNames(events)
{
}

PrintSystemObjectPropertiesChangedEventArgs::
~PrintSystemObjectPropertiesChangedEventArgs(
    void
    )
{
}

StringCollection^
PrintSystemObjectPropertiesChangedEventArgs::PropertiesNames::
get(
    void
    )
 {
     return propertiesNames;
 }
