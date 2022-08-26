#pragma once

#ifndef __GENERICDRIVERTHUNKFILTER_HPP__
#define __GENERICDRIVERTHUNKFILTER_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:    

        GenericDriverThunkFilter.hpp
        
    Abstract:

        DriverThunkingProfile - This object holds the knowledge about how a Driver object
        thunks into unmanaged code. It does the mapping between the attributes 
        and Win32 levels, it does the level reconciliation and based on a 
        coverage mask, it creates the coverage list.
        
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
namespace AttributeNameToInfoLevelMapping
{
namespace DriverThunk
{

    private ref class DriverThunkingProfile sealed : public IThunkingProfile
    {
        static
        DriverThunkingProfile(
            void
            )
        {
            attributeMap = gcnew Hashtable();
            RegisterAttributeMap();
        }

        public:

        virtual InfoLevelCoverageList^
        GetCoverageList(
            InfoLevelMask    coverageMask
            );

        static
        Hashtable^
        GetStaticAttributeMap(
            void
            );

        static
        UInt64
        ReconcileMask(
            UInt64           coverageMask
            );
        
        private:
        
        static
        void
        RegisterAttributeMap(
            void
            );

        static
        Hashtable^      attributeMap;

        static
        array<InfoLevelMask>^ levelMaskTable = 
        {
            InfoLevelMask::NoLevel,
            InfoLevelMask::LevelOne,
            InfoLevelMask::LevelTwo,
            InfoLevelMask::LevelThree,
            InfoLevelMask::LevelFour,
            InfoLevelMask::LevelFive,
            InfoLevelMask::LevelSix,
            InfoLevelMask::LevelSeven
        };
    };
}
}
}
}
}
#endif
