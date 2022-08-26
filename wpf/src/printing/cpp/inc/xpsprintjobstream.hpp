#pragma once

#ifndef __XPSPRINTJOBSTREAM_HPP__
#define __XPSPRINTJOBSTREAM_HPP__
/*++
                                                                              
    Copyright (C) 2002 - 2011 Microsoft Corporation                                   
    All rights reserved.                                                        
                                                                              
    Module Name:   

        XpsPrintJobStream.hpp
        
    Abstract:

        Managed wrapper for IXpsPrintJobStream interface.

    Author: 

        Ifeanyi Echeruo (Microsoft) May 7th 2010
                                                                             
    Revision History:  
--*/

namespace MS
{
namespace Internal
{
namespace PrintWin32Thunk
{
    using namespace System;
    using namespace System::IO;
    using namespace System::Security;
    using namespace System::Security::Permissions;
    
    private ref class XpsPrintJobStream : public Stream
    {
        public:
            
		/// <SecurityNote>
		///	SecurityCritical - Sets critical memeber inner
		/// </SecurityNote>
		[SecurityCritical]
        XpsPrintJobStream(
            void /* IXpsPrintJobStream */ *printJobStream, // ilasm does not recognise IXpsPrintJobStream
            System::Threading::ManualResetEvent^ hCompletedEvent,
            Boolean canRead,
            Boolean canWrite
        );
        
		/// <SecurityNote>
		///	Critical - Calls native method release com object to close print stream
        ///     TreatAsSafe - Demands default printing
		/// </SecurityNote>
		[SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
        SecurityAction::Demand,
        Level=System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]        
        ~XpsPrintJobStream();
        
		/// <SecurityNote>
		///	Critical - Calls native method release com object to close print stream
        ///     TreatAsSafe - Demands default printing
		/// </SecurityNote>
		[SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
        SecurityAction::Demand,
        Level=System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]        
        !XpsPrintJobStream();
        
        property
        Boolean
        CanRead
        {
            Boolean virtual get() override;
        }

        property
        Boolean
        CanWrite
        {
            Boolean virtual get() override;
        }
        
        property
        Boolean
        CanSeek
        {
            Boolean virtual get() override;
        }

        property
        Boolean
        CanTimeout
        {
            Boolean virtual get() override;
        }

        property
        Int64
        Length
        {
            Int64 virtual get() override;
        }
        
        property
        Int64
        Position
        {
            Int64 virtual get() override;
            void virtual set(Int64 value) override;
        }
        
        virtual
        void 
        Flush (
            void
        ) override ;
        
		/// <SecurityNote>
		///	Critical - Calls native method to read spool stream from printer device
		/// </SecurityNote>
		[SecurityCritical]
        virtual
        Int32
        Read (
            array<Byte> ^ buffer,
            Int32 offset,
            Int32 count
        ) override;
        
		/// <SecurityNote>
		///	Critical - Calls native method to write untrusted data to printer device
		/// </SecurityNote>
		[SecurityCritical]
        virtual
        void
        Write (
            array<Byte> ^ buffer,
            Int32 offset,
            Int32 count
        ) override;
        
        virtual
        Int64
        Seek (
            Int64 offset,
            SeekOrigin origin
        ) override;
        
        virtual
        void
        SetLength (
            Int64 value
        ) override;
        
        private:

        /// <Summary>
        /// Wrapper around WaitForSingleObjectEx that hides away its various return codes
        /// </Summary>
		/// <SecurityNote>
		///	Critical    - Waits on critical member hCompletedEvent
        /// TreatAsSafe - Demands default printing
		/// </SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
        SecurityAction::Demand,
        Level=System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]        
        BOOL 
        WaitForJobCompletion (
            DWORD waitTimeout
        );

        DWORD
        GetCommitTimeoutMilliseconds (
            void
        );

		/// <SecurityNote>
		///	Critical - Pointer to COM interface used to write data to printer device
		/// </SecurityNote>
		[SecurityCritical]
        IXpsPrintJobStream* inner;

		/// <SecurityNote>
		///	Critical - Waithandle used to wait for critical 'inner' to be safe to release after closing
		/// </SecurityNote>		
		[SecurityCritical]            
        System::Threading::ManualResetEvent^ hCompletedEvent;
		
        Boolean canRead;
        Boolean canWrite;
        Int64 position;
    };
}
}
}

#endif
