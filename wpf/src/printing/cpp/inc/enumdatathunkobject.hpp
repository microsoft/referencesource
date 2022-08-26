#pragma once

#ifndef __ENUMDATATHUNKOBJECT_HPP__
#define __ENUMDATATHUNKOBJECT_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                

        EnumDataThunkObject.hpp
        
    Abstract:

        This file contains the declaration for EnumDataThunkObject object.
        This object enumerates the objects of a given type by calling Win32 APIs. 
        The Win32 APIs to be called are determined based on the propertiesFilter
        parameter. The objects are created and only the properties in the propertiesFilter
        are populated with data. The objects are added to the printObjectsCollection.
        
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
    private ref class EnumDataThunkObject
    {
        public:

        EnumDataThunkObject(
            Type^       printingType
            );

        ~EnumDataThunkObject(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Release on InfoLevelCoverageList.
        /// TreatAsSafe - InfoLevelCoverageList created in the method and not exposed.
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        GetPrintSystemValuesPerPrintQueues(
            PrintServer^                        printServer,
            array<EnumeratedPrintQueueTypes>^   flags,
            System::
            Collections::
            Generic::Queue<PrintQueue^>^        printObjectsCollection,
            array<String^>^                     propertyFilter
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Release on InfoLevelCoverageList.
        /// TreatAsSafe - InfoLevelCoverageList created in the method and not exposed.
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        GetPrintSystemValuesPerPrintJobs(
            PrintQueue^                   printQueue,
            System::
            Collections::
            Generic::
            Queue<PrintSystemJobInfo^>^   printObjectsCollection,
            array<String^>^               propertyFilter,
            UInt32                        firstJobIndex,
            UInt32                        numberOfJobs
            );

        private:

        EnumDataThunkObject(
            void
            );

        UInt32
        TweakTheFlags(
            UInt32  attributeFlags
            );

        AttributeNameToInfoLevelMapping::
        InfoLevelCoverageList^
        BuildCoverageListAndEnumerateData(
            String^                                             serverName,
            UInt32                                              flags,            
            AttributeNameToInfoLevelMapping::InfoLevelMask      mask          
            );

        AttributeNameToInfoLevelMapping::
        InfoLevelCoverageList^
        BuildJobCoverageListAndEnumerateData(
            PrinterThunkHandler^                                    printingHandler,  
            AttributeNameToInfoLevelMapping::InfoLevelMask          mask,
            UInt32                                                  firstJobIndex,
            UInt32                                                  numberOfJobs
            );

        void
        MapEnumeratePrinterQueuesFlags(
            array<EnumeratedPrintQueueTypes>^                       enumerateFlags
            );

        Type^                                printingType;    
        bool                                 isDisposed;  
        UInt32                               win32EnumerationFlags;
        UInt32                               win32PrinterAttributeFlags;
    };
}
}
}
#endif
