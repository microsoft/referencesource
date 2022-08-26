#ifndef __PORT_HPP__
#define __PORT_HPP__

/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                
        Port.hpp                                                             
                                                                              
    Abstract:
        
    Author:                                                                     
        Khaled Sedky (khaleds) 10-December-2002                                        
     
                                                                             
    Revision History:                                                           
--*/

namespace System
{
namespace Printing
{
    public ref class PrintPort sealed :
    public PrintSystemObject
    {
        public:

        virtual void
        Commit(
            void
            ) override;

        virtual void
        Refresh(
            void
            ) override;
       
        internal: 

        PrintPort(
            String^    portName
        );

        virtual PrintPropertyDictionary^
        get_InternalPropertiesCollection(
            String^ attributeName
            ) override;

        static
        void
        RegisterAttributesNamesTypes(
            void
            );

        static
        PrintProperty^
        CreateAttributeNoValue(
            String^
            );

        static
        PrintProperty^
        CreateAttributeValue(
            String^,
            Object^
            );

        static
        PrintProperty^
        CreateAttributeNoValueLinked(
            String^,
            MulticastDelegate^
            );

        static
        PrintProperty^
        CreateAttributeValueLinked(
            String^,
            Object^,
            MulticastDelegate^
            );

        protected:

        virtual
        void
        InternalDispose(
            bool disposing
            ) override sealed;

        private:

        static 
        PrintPort(
            void
            )
        {
            attributeNameTypes = gcnew Hashtable();
        }

        void
        VerifyAccess(
            void
        );


        static Hashtable^ attributeNameTypes;
        PrintSystemDispatcherObject^  accessVerifier;

    };
}
}

#endif
