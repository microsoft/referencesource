#pragma once

#ifndef __GETDATATHUNKOBJECT_HPP__
#define __GETDATATHUNKOBJECT_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:     

        GetDataThunkObject.hpp
        
    Abstract:

        This file contains the declaration for GetDataThunkObject object.
        This object populates the PrintSystemObject with data by calling Win32 APIs. 
        The Win32 APIs to be called are determined based on the propertiesFilter
        parameter. 
        
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
    private ref class GetDataThunkObject
    {
        public:

        GetDataThunkObject(
            Type^       printingType
            );

        ~GetDataThunkObject(
            void
            );
        
        //
        // Populates an AttributeValue collection for a given type by 
        // calling the Win32 Get method.
        //
        bool
        PopulatePrintSystemObject(
            PrinterThunkHandler^                   printingHandler,
            PrintSystemObject^                     printObject,
            array<String^>^                        propertiesFilter
            );

        
        property
        Object^
        Cookie
        {
            void set(Object^ internalCookie);
            Object^ get();
        }

        private:

        GetDataThunkObject(
            void
            );

        AttributeNameToInfoLevelMapping::InfoLevelCoverageList^
        BuildCoverageListAndGetData(
            PrinterThunkHandler^                                        printingHandler,
            AttributeNameToInfoLevelMapping::InfoLevelMask              mask       
            );

        bool
        PopulateAttributesFromCoverageList(
            PrintSystemObject^                                          printObject,
            array<String^>^                                             propertiesFilter,
            AttributeNameToInfoLevelMapping::InfoLevelCoverageList^     coverageList
            );
               

        Type^       printingType;    
        bool        isDisposed;  
        Object^     cookie;
    };
}
}
}
#endif
