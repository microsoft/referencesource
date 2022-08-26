//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;

    class LogLogRecord : LogRecord
    {
        RecordStream stream;
        SequenceNumber sequenceNumber;
        SequenceNumber previous;
        SequenceNumber user;

        unsafe internal LogLogRecord(
            SequenceNumber sequenceNumber,
            SequenceNumber user,
            SequenceNumber previous,
            byte* data,
            long length)
        {
            this.sequenceNumber = sequenceNumber;
            this.previous = previous;
            this.user = user;

            if (length < LogLogRecordHeader.Size)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
            }

            byte[] headerBits = new byte[LogLogRecordHeader.Size];
            Marshal.Copy(new IntPtr(data),
                         headerBits,
                         0,
                         LogLogRecordHeader.Size);

            LogLogRecordHeader header = new LogLogRecordHeader(headerBits);
            if (header.MajorVersion > LogLogRecordHeader.CurrentMajorVersion)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.IncompatibleVersion());
            }

            length -= LogLogRecordHeader.Size;
            data += LogLogRecordHeader.Size;
            if (header.Padding)
            {
                long paddingSize = LogLogRecordHeader.DecodePaddingSize(data, length);
                if ((paddingSize < 0) || (paddingSize > length))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
                }

                length -= paddingSize;
                data += paddingSize;
            }

            this.stream = new RecordStream(
                new UnmanagedMemoryStream(data, length));
        }

        public override Stream Data
        {
            get
            {
                return this.stream;
            }
        }

        public override SequenceNumber Previous
        {
            get
            {
                return this.previous;
            }
        }

        public override SequenceNumber SequenceNumber
        {
            get
            {
                return this.sequenceNumber;
            }
        }

        public override SequenceNumber User
        {
            get
            {
                return this.user;
            }
        }

        public override void Dispose()
        {
            this.stream.Dispose();
        }

        internal void Detach()
        {
            this.stream.MakeLocalCopy();
        }

        class RecordStream : Stream
        {
            Stream innerStream;
            object syncRoot;

            internal RecordStream(Stream innerStream)
            {
                this.innerStream = innerStream;
                this.syncRoot = new object();
            }

            public override bool CanRead { get { return true; } }
            public override bool CanSeek { get { return true; } }
            public override bool CanWrite { get { return false; } }

            public override long Length
            {
                get
                {
                    lock (this.syncRoot)
                    {
                        CheckDisposed();
                        return this.innerStream.Length;
                    }
                }
            }

            public override long Position
            {
                get
                {
                    lock (this.syncRoot)
                    {
                        CheckDisposed();
                        return this.innerStream.Position;
                    }
                }

                set
                {
                    lock (this.syncRoot)
                    {
                        CheckDisposed();
                        this.innerStream.Position = value;
                    }
                }
            }

            public override void Flush()
            {
                lock (this.syncRoot)
                {
                    if (this.innerStream != null)
                    {
                        this.innerStream.Flush();
                    }
                }
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                lock (this.syncRoot)
                {
                    CheckDisposed();
                    return this.innerStream.Read(buffer, offset, count);
                }
            }

            public override long Seek(long offset, SeekOrigin origin)
            {
                lock (this.syncRoot)
                {
                    CheckDisposed();
                    return this.innerStream.Seek(offset, origin);
                }
            }

            public override void SetLength(long value)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                if (this.innerStream == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }
                else
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
                }
            }

            public override void WriteByte(byte value)
            {
                if (this.innerStream == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }
                else
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
                }
            }

            internal void MakeLocalCopy()
            {
                lock (this.syncRoot)
                {
                    if (this.innerStream != null)
                    {
                        long originalPosition = this.innerStream.Position;
                        try
                        {
                            try
                            {
                                byte[] newData = new byte[this.innerStream.Length];
                                this.innerStream.Position = 0;
                                this.innerStream.Read(newData, 0, newData.Length);

                                Stream oldStream = this.innerStream;
                                this.innerStream = new MemoryStream(newData);
                                oldStream.Close();
                            }
                            finally
                            {
                                this.innerStream.Position = originalPosition;
                            }
                        }
                        catch
                        {
                            // This outer catch block is needed to prevent an exception filter
                            // from running while we're in an inconsistent state before
                            // restoring the inner stream's original position.
                            // See http://msdn2.microsoft.com/en-US/library/8cd7yaws.aspx
                            throw;
                        }
                    }
                }
            }

            protected override void Dispose(bool disposing)
            {
                try
                {
                    if (disposing)
                    {
                        lock (this.syncRoot)
                        {
                            if (this.innerStream != null)
                            {
                                this.innerStream.Close();
                                this.innerStream = null;
                            }
                        }
                    }
                }
                finally
                {
                    base.Dispose(disposing);
                }
            }

            void CheckDisposed()
            {
                if (this.innerStream == null)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }
            }
        }
    }
}
