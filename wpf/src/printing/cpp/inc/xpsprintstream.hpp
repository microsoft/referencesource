#pragma once

#ifndef __XPSPRINTSTREAM_HPP__
#define __XPSPRINTSTREAM_HPP__
/*++

Copyright (C) 2002 - 2016 Microsoft Corporation
    All rights reserved.

    Module Name:

        XpsPrintStream.hpp

    Abstract:

        Managed wrapper for IStream interface.

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
    using namespace System::Printing;
    using namespace System::Runtime::InteropServices;
    using namespace System::Security;
    using namespace System::Security::Permissions;
    using namespace System::Runtime::InteropServices;
    
    private ref class XpsPrintStream : public Stream
    {
    
    private:
        /// <SecurityNote>
        /// SecurityCritical - Sets critical member inner
        /// </SecurityNote>
        [SecurityCritical]
        XpsPrintStream(
            void /* IStream */ *IPrintStream,
            Boolean canRead,
            Boolean canWrite
            );

    public:
        /// <SecurityNote>
        /// Critical - Calls Release on critical member _innerStream
        /// TreatAsSafe - Does not expose any critical data 
        /// </SecurityNote>
        [SecuritySafeCritical]
        ~XpsPrintStream();

        /// <SecurityNote>
        /// Critical - Releases critical member _innerStream
        /// TreatAsSafe - Does not expose any critical data
        /// </SecurityNote>
        [SecuritySafeCritical]
        !XpsPrintStream();

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
            Flush(
            void
            ) override;

        /// <SecurityNote>
        /// Critical - Calls native method to read stream contents
        /// </SecurityNote>
        [SecurityCritical]
        virtual
            Int32
            Read(
            array<Byte> ^ buffer,
            Int32 offset,
            Int32 count
            ) override;

        /// <SecurityNote>
        /// Critical - Calls native method to write untrusted data to critical member _innerStream
        /// </SecurityNote>
        [SecurityCritical]
        virtual
            void
            Write(
            array<Byte> ^ buffer,
            Int32 offset,
            Int32 count
            ) override;

        /// <SecurityNote>
        /// Critical - Calls native method to set the stream position
        /// TreatAsSafe - Does not expose any critical data
        /// </SecurityNote>
        [SecuritySafeCritical]
        virtual
            Int64
            Seek(
            Int64 offset,
            SeekOrigin origin
            ) override;

        virtual
            void
            SetLength(
            Int64 value
            ) override;

        /// <SecurityNote>
        /// Critical - Calls into native method to create an IStream and into critical constructor
        /// TreatAsSafe - Does not expose any critical data
        /// </SecurityNote>
        [SecuritySafeCritical]
        [SecurityPermission(SecurityAction::Assert, UnmanagedCode = true)]
        static
        XpsPrintStream^
        CreateXpsPrintStream();

        /// <SecurityNote>
        /// Critical - Exposes a managed handle to critical member innerStream
        /// </SecurityNote>
        [SecurityCritical]
        ComTypes::IStream^
        GetManagedIStream();

        private:

        /// <SecurityNote>
        /// Critical - Pointer to COM interface used to write data to printer device
        /// </SecurityNote>
        [SecurityCritical]
        IStream* _innerStream;

        Boolean _canRead;
        Boolean _canWrite;
        Int64 _position;
    };
}
}
}

#endif
