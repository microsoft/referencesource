#ifndef __PRINTPROCESSOR_HPP__
#define __PRINTPROCESSOR_HPP__

/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                
        PrintProcessor.hpp                                                             
                                                                              
    Abstract:
        
    Author:                                                                     
        Adina Trufinescu (AdinaTru) 10-January-2002
                                                                             
    Revision History:                                                           
        Khaled Sedky (khaleds) 15-January-2002                                        
--*/

namespace System
{
namespace Printing
{
    /// <summary>
    ///    This class abstracts the functionality of a print processor.
    ///    This object is returned by the Print System and cannot be instantiated by the end user.
    ///    The object has minimal functionality as it stands today. It is considered a management object.
    /// </summary>
    /// <ExternalAPI/>
    public ref class PrintProcessor sealed :
    public PrintFilter
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

        PrintProcessor(
            String^    printProcessorName
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
            bool    disposing
            ) override sealed;

        private:

        static 
        PrintProcessor(
            void
            )
        {
            attributeNameTypes = gcnew Hashtable();
        }

        void
        VerifyAccess(
            void
        );

        PrintSystemDispatcherObject^    accessVerifier;
        static Hashtable^ attributeNameTypes;
    };
}
}

#endif
