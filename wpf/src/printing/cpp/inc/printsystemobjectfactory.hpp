#ifndef __PRINTSYSTEMOBJECTFACTORY_HPP__
#define __PRINTSYSTEMOBJECTFACTORY_HPP__

/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                
        PrintSystemObjectFactory.hpp                                                             
                                                                              
    Abstract:
        This file contains the declaration of the 
        classes representing Factory methods used to
        create instances of different Print System 
        objects
        
    Author:                                                                     
        Khaled Sedky (khaleds) 20-November-2002                                        
     
                                                                             
    Revision History:                                                           
--*/

namespace System
{
namespace Printing
{
namespace Activation
{
    private ref  class PrintSystemObjectFactory sealed : 
    public IDisposable 
    {
        public:

        void
        RegisterInstantiationDelegates(
            Type^                                   objType,
            PrintSystemObject::Instantiate^         instantiationDelegate
            );

        void
        RegisterOptimizedInstantiationDelegates(
            Type^                                     objType,
            PrintSystemObject::InstantiateOptimized^  optimizedInstantiationDelegate
            );

        PrintSystemObject^
        Instantiate(
            Type^           objType,
            array<String^>^ propertiesFilter
            );

        PrintSystemObject^
        InstantiateOptimized(
            Type^           objType,
            Object^         object,
            array<String^>^ propertiesFilter
            );

        property
        static
        PrintSystemObjectFactory^
        Value
        {
            PrintSystemObjectFactory^ get();
        }

        protected:

        !PrintSystemObjectFactory(
            void
            );

        virtual
        void
        InternalDispose(
            bool disposing
            );

        private:

        static 
        PrintSystemObjectFactory(
            void
            )
        {
            PrintSystemObjectFactory::value    = nullptr;
            PrintSystemObjectFactory::syncRoot = gcnew Object();
        }
        
        PrintSystemObjectFactory(
            void
            );

        ~PrintSystemObjectFactory(
            void
            );

        property
        static
        Object^
        SyncRoot
        {
            Object^ get();
        }

        static 
        PrintSystemObjectFactory^   value;

        static
        volatile
        Object^                     syncRoot;

        bool                        disposed;
        Hashtable^                  instantiationDelegatesTable;
        Hashtable^                  optimizedInstantiationDelegatesTable;
    };
}
}
}

#endif
