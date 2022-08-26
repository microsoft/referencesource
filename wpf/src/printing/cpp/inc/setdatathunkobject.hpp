#pragma once

#ifndef __SETDATATHUNKOBJECT_HPP__
#define __SETDATATHUNKOBJECT_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                

        SetDataThunkObject.hpp
        
    Abstract:
    
        This file contains the declaration for SetDataThunkObject object.
        This object commits the dirty data in the PrintSystemObject by calling Win32 APIs. 
        The propertiesFilter specify the set of dirty properties.
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
    private ref class SetDataThunkObject
    {
        public:

        SetDataThunkObject(
            Type^       printingType
            );

        ~SetDataThunkObject(
            void
            );

        bool
        CommitDataFromPrintSystemObject(
            PrinterThunkHandler^                                    printingHandler,
            PrintSystemObject^                                      printObject,
            array<String^>^                                         propertiesFilter
            );
        
        private:

        AttributeNameToInfoLevelMapping::InfoLevelCoverageList^
        BuildCoverageListToSetData(
            PrinterThunkHandler^                                    printerThunkHandler,
            AttributeNameToInfoLevelMapping::InfoLevelMask          mask
            );
        
        bool
        SetAttributesFromCoverageList(
            PrintSystemObject^                                      printObject,
            array<String^>^                                         propertiesFilter,
            AttributeNameToInfoLevelMapping::InfoLevelCoverageList^ coverageList                         
            );

        bool
        SetDataFromCoverageList(
            PrinterThunkHandler^                                    printingHandler,
            array<String^>^                                         propertiesFilter,
            AttributeNameToInfoLevelMapping::InfoLevelCoverageList^ coverageList,
            Type^                                                   setDataType    
            );

        void
        GetCommitedAndFailedAttributes(
            array<String^>^                                         propertiesFilter,
            AttributeNameToInfoLevelMapping::InfoLevelCoverageList^ coverageList,
            System::Collections::ObjectModel::Collection<String^>^  committedAttributes,
            System::Collections::ObjectModel::Collection<String^>^  failedAttributes
            );

        ///<SecurityNote>
        ///  Critical    : References type from non-APTCA reachframework.dll
        ///  TreatAsSafe : Type is safe exception type
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        Exception^
        CreatePrintCommitAttributesException (
            int                  hResult,
            System::Collections::ObjectModel::Collection<String^>^ committedAttributes,
            System::Collections::ObjectModel::Collection<String^>^ failedAttributes
            );

        Type^    printingType;    
        bool     isDisposed;  
    };
    
}
}
}
#endif
