#pragma once

#ifndef __INTEROPATTRIBUTEVALUEDICTIONARY_HPP__
#define __INTEROPATTRIBUTEVALUEDICTIONARY_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                

        InteropAttributeValueDictionary.hpp
        
    Abstract:

        Utility classes that allocate and free the unmanaged printer info
        buffers that are going to be sent to the Win32 APIs.
        
    Author: 

        Adina Trufinescu (AdinaTru) December 02th 2003
                                                                             
    Revision History:  
--*/
namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
    using namespace  System::Printing::IndexedProperties;

    private ref class AttributeValueInteropHandler
    {
        public:
        
        enum class PrintPropertyTypeInterop
        {
            StringPrintType     = kPropertyTypeString, //1
            Int32PrintType      = kPropertyTypeInt32, //2            
            DataTimePrintType   = kPropertyTypeTime,//5
            ByteBufferPrintType = 10,//kPropertyTypeBuffer
        };

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access
        ///</SecurityNote>
        [SecurityCritical]
        IntPtr
        BuildUnmanagedPrintPropertiesCollection(
            PrintPropertyDictionary^    collection
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollection is cast to a pointer and read)
        ///</SecurityNote>
        [SecurityCritical]
        PrintPropertyDictionary^
        BuildManagedPrintPropertiesCollection(
            IntPtr                      unmanagedCollection
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollection is cast to a pointer and read)
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        FreeUnmanagedPrintPropertiesCollection(
            IntPtr  unmanagedCollection
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollection is cast to a pointer and read)
        ///</SecurityNote>
        [SecurityCritical]
        void
        CopyManagedPrintPropertiesCollection(
            IntPtr                    unmanagedCollection,
            PrintSystemObject^        printSystemObject
            );

        ///<SecurityNote>
        /// Critical    : Contains unverifiable code
        ///               -  Performs reinterpret cast (handle to pointer and pointer to IntPtr) 
        ///               -  Calls native method (LocalAlloc) 
	///		-  Returns IntPtr to allocated native memory
        ///<SecurityNote>
        [SecurityCritical]
        static
        IntPtr
        AllocateUnmanagedPrintPropertiesCollection(
            Int32     propertyCount
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollectionPtr is cast to a pointer and written)
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        SetValue(
            IntPtr          unmanagedCollectionPtr,
            String^         propertyName,
            UInt32          index,
            System::Type^   type
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollectionPtr is cast to a pointer and written)
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        SetValue(
            IntPtr      unmanagedCollectionPtr,
            String^     propertyName,
            UInt32      index,
            Object^     value
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access (unmanagedCollection is cast to a pointer and read)
        ///</SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetValue(
            IntPtr                      unmanagedCollectionPtr,
            String^                     propertyName,
            Type^                       type,
            Boolean%                    isPropertyPresent
            );

        property
        static
        AttributeValueInteropHandler^
        Value
        {
            AttributeValueInteropHandler^ get();
        }

        private:

        static
        Hashtable^                      unmanagedToManagedTypeMap;

        static
        Hashtable^                      managedToUnmanagedTypeMap;

        static
        Hashtable^                      attributeValueToUnmanagedTypeMap;

        static
        Hashtable^                      unmanagedPropertyToObjectDelegateMap;

        static 
        volatile 
        AttributeValueInteropHandler^      value;

        static
        volatile
        Object^                            syncRoot; 

        property
        static
        Object^
        SyncRoot
        {
            Object^ get();
        }

        delegate
        Object^
        GetValueFromUnmanagedValue(
            PrintPropertyValue                 unmanagedPropertyValue
            );

        ///<SecurityNote>
        /// Critical:   Calls critical String::String(Char*)
        ///             Unverifiable pointer access
        ///</SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetString(
            PrintPropertyValue                 unmanagedPropertyValue
            );

        ///<SecurityNote>
        /// Critical:   C++ compiler emits pointer based IL instructions and annotated the method with the unsafe keyword in generated code
        ///</SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetInt32(
            PrintPropertyValue                 unmanagedPropertyValue
            );

        ///<SecurityNote>
        /// Critical:   Calls critical Marshal::Copy
        ///             Unverifiable pointer access
        ///</SecurityNote>
        [SecurityCritical]
        static
        Object^
        GetStream(
            PrintPropertyValue                 unmanagedPropertyValue
            );

        static
        Object^
        GetDateTime(
            PrintPropertyValue                 unmanagedPropertyValue
            );

        static array<Type^>^ PrintSystemAttributePrimitiveTypes = 
        {
            String::typeid,
            Int32::typeid,            
            DateTime::typeid,
            System::IO::MemoryStream::typeid
        };

        static array<Type^>^ PrintSystemAttributeValueTypes = 
        {
            PrintStringProperty::typeid,
            PrintInt32Property::typeid,
            PrintDateTimeProperty::typeid,
            PrintStreamProperty::typeid
        };

        static array<GetValueFromUnmanagedValue^>^ GetValueFromUnmanagedValueDelegateTable = 
        {
            gcnew GetValueFromUnmanagedValue(&GetString),
            gcnew GetValueFromUnmanagedValue(&GetInt32),
            gcnew GetValueFromUnmanagedValue(&GetDateTime),
            gcnew GetValueFromUnmanagedValue(&GetStream)
        };


        AttributeValueInteropHandler(
            void
            );

        ///<SecurityNote>
        /// Critical    :   References critical methods
        ///             :   See GetValueFromUnmanagedValueDelegateTable static initializer
        /// TreatAsSafe :   Does not execute methods (places methods in a collection of delegates)
        ///</SecurityNote>
        [SecuritySafeCritical]
        static 
        AttributeValueInteropHandler(
            void
            )
        {
            
            AttributeValueInteropHandler::value    = nullptr;
            AttributeValueInteropHandler::syncRoot = gcnew Object();
        
            managedToUnmanagedTypeMap            = gcnew Hashtable;
            unmanagedToManagedTypeMap            = gcnew Hashtable;
            attributeValueToUnmanagedTypeMap     = gcnew Hashtable;
            unmanagedPropertyToObjectDelegateMap = gcnew Hashtable;
            

            RegisterStaticMaps();
        }

        static 
        void
        RegisterStaticMaps(
            void
            );
                
        ///<SecurityNote>
        /// Critical    : Calls critical AllocateUnmanagedPrintPropertiesCollection
	///		-  Returns IntPtr to allocated native memory
        ///<SecurityNote>
	[SecurityCritical]
        IntPtr
        AllocateUnmanagedPrintPropertiesCollection(
            PrintPropertyDictionary^    managedCollection
            );

        ///<SecurityNote>
        /// Critical: Unverifiable pointer access
        ///</SecurityNote>
        [SecurityCritical]
        void
        AssignUnmanagedPrintPropertyValue(
            PrintNamedProperty*  unmanagedPropertyValue,
            PrintProperty^       managedAttributeValue
            );
          
    };

}
}
}
#endif

