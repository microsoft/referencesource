#ifndef __PRINTSYSTEMNOTIFICATIONS_HPP__
#define __PRINTSYSTEMNOTIFICATIONS_HPP__

/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   
        PrintSystemNotifications.hpp                                                             
                                                                              
    Abstract:
        
    Author:                                                                     
        Adina Trufinescu (AdinaTru) 10-January-2002                                        
     
    Revision History:             

        Khaled Sedky (KhaledS) 12-January-2002
--*/

namespace System
{
namespace Printing
{
    public ref class PrintSystemObjectPropertyChangedEventArgs: 
    public EventArgs
    {
        public:

        PrintSystemObjectPropertyChangedEventArgs(
            String^ eventName
            );

        ~PrintSystemObjectPropertyChangedEventArgs(
            void
            );

        property
        String^
        PropertyName
        {
            String^ get();
        }
    
        protected:

        private:

        String^     propertyName;
    };

    public ref class PrintSystemObjectPropertiesChangedEventArgs: 
    public EventArgs
    {
        public:

        PrintSystemObjectPropertiesChangedEventArgs(
            StringCollection^   events
            );

        ~PrintSystemObjectPropertiesChangedEventArgs(
            void
            );

        property
        StringCollection^
        PropertiesNames
        {
            StringCollection^ get();
        }
    
        protected:

        private:

        StringCollection^     propertiesNames;
    };
}
}

#endif
