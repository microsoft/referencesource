#ifndef __INTERNALPRINTSYSTEMEXCEPTION_HPP__
#define __INTERNALPRINTSYSTEMEXCEPTION_HPP__

/*++
                                                                              
    Copyright (C) 2002 - 2003 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:                                                                

        InternalPrintSystemException.hpp                                                             
                                                                              
    Abstract:
        
        Print System exception objects declaration.

    Author:                                                                     
            
        Adina Trufinescu June 3rd 2003
                                                                             
    Revision History:                                                           
--*/

#pragma once

namespace System
{
namespace Printing
{
    using namespace System::Security;
    
    private ref class InternalPrintSystemException
    {
        internal:

        InternalPrintSystemException(
            int   lastWin32Error
            );

        // FIX: remove pragma. done to fix compiler error which will be fixed later.
        #pragma warning ( disable:4376 )
        property
        int
        HResult
        {
            int get();
        }

        static
        void
        ThrowIfErrorIsNot(
            int     lastWin32Error,
            int     expectedLastWin32Error
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Marshal.GetLastWin32Error()
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        ThrowIfLastErrorIsNot(
            int     expectedLastWin32Error
            );

        ///<SecurityNote>
        /// Critical    - Calls critical Marshal.GetLastWin32Error()
        ///</SecurityNote>
        [SecurityCritical]
        static
        void
        ThrowLastError(
            void
            );

        static
        void
        ThrowIfNotSuccess(
            int     lastWin32Error
            );

        static
		void
		ThrowIfNotCOMSuccess(
		    HRESULT  hresultCode
		    );
        private:
        
        int   hresult;

        static
        const 
        int  defaultWin32ErrorMessageLength = 256;
    };

    }
}
#endif

