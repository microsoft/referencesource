//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace System.IO.Log
{

    // This class is not thread safe.
    // FileLogRecordStream lets the user read log records in the form of a stream.
    internal class FileLogRecordStream : System.IO.Stream
    {
        // The stream initialy reads the first 4K bytes of the log record.  
        // When need arises, it reads the entire record and creates a new Memory stream with the record data. 
        const int INITIAL_READ = 4096;

        // The memory stream is created from the record data.  It handles all stream operations.
        MemoryStream stream;
        SequenceNumber recordSequenceNumber;
        SimpleFileLog log;
        FileLogRecordHeader header;

        // Sequence numbers of the previous and next records.  Returned from ReadRecord and ReadRecordPrefix.
        SequenceNumber prevSeqNum;
        SequenceNumber nextSeqNum;
        int recordSize;
        bool entireRecordRead;
        object streamLock;

        internal FileLogRecordStream(SimpleFileLog log, SequenceNumber recordSequenceNumber)
        {
            this.log = log;
            this.recordSequenceNumber = recordSequenceNumber;
            this.stream = null;
            this.recordSize = 0;
            this.entireRecordRead = false;
            this.streamLock = new object();

            CreateMemoryStream();
        }

        // Creates a memory stream with the first 4K bytes of the record.
        private void CreateMemoryStream()
        {
            byte[] pData;
            int cbData = INITIAL_READ;
            int totalSize;

            log.ReadRecordPrefix(this.recordSequenceNumber, out pData, ref cbData, out totalSize, out this.prevSeqNum, out this.nextSeqNum);

            if (cbData < FileLogRecordHeader.Size)
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());

            this.header = new FileLogRecordHeader(pData);
            this.recordSize = totalSize - FileLogRecordHeader.Size;

            int streamSize = Math.Min(this.recordSize,
                                      INITIAL_READ - FileLogRecordHeader.Size);

            this.stream = new MemoryStream(
                pData,
                FileLogRecordHeader.Size,
                streamSize,
                false);

            if (totalSize <= INITIAL_READ)
            {
                // Record is smaller than 4K.  We have read the entire record.
                this.entireRecordRead = true;
            }
        }

        internal FileLogRecordHeader Header
        {
            get { return this.header; }
        }

        internal SequenceNumber RecordSequenceNumber
        {
            get { return this.recordSequenceNumber; }
        }

        internal SequenceNumber PrevLsn
        {
            get { return this.prevSeqNum; }
        }

        internal SequenceNumber NextLsn
        {
            get { return this.nextSeqNum; }
        }

        private MemoryStream Stream
        {
            get { return this.stream; }
        }

        // Multiple threads in Read can result in corrupt data.
        public override int Read(byte[] buffer, int offset, int count)
        {
            int bytesRead = this.Stream.Read(buffer, offset, count);

            if (bytesRead == count || this.entireRecordRead)
            {
                return bytesRead;
            }
            else
            {
                // We have reached the end of the stream.  Read the entire record and create a new stream.
                ReadEntireRecord();

                return this.Stream.Read(buffer, offset + bytesRead, count - bytesRead) + bytesRead;
            }
        }

        // The method reads the entire record and creates a new memory stream.  
        private void ReadEntireRecord()
        {
            lock (this.streamLock)
            {
                if (this.entireRecordRead)
                    return;

                // Check if the stream is open.
                if (!this.stream.CanRead)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }

                byte[] data;
                int size;
                SequenceNumber prev;
                SequenceNumber next;

                log.ReadRecord(this.recordSequenceNumber, out data, out size, out prev, out next);
                if (!(size == (this.recordSize + FileLogRecordHeader.Size)
                   && next == this.nextSeqNum))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
                }

                long position = this.Stream.Position;

                // Create a new stream
                this.stream = new MemoryStream(
                    data,
                    FileLogRecordHeader.Size,
                    data.Length - FileLogRecordHeader.Size,
                    false);

                this.Stream.Position = position;
                this.entireRecordRead = true;
            }
        }

        public override bool CanRead
        {
            get { return this.stream.CanRead; }
        }

        public override bool CanSeek
        {
            get { return this.stream.CanSeek; }
        }

        public override bool CanWrite
        {
            get { return this.stream.CanWrite; }
        }

        public override void Close()
        {
            lock (this.streamLock)
            {
                this.stream.Close();
            }
        }

        public override void Flush()
        {
            this.Stream.Flush();
        }

        public override long Length
        {
            get
            {
                if (!this.stream.CanRead)
                {
#pragma warning suppress 56503
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
                }

                return this.recordSize;
            }
        }

        public override long Position
        {
            get
            {
                return this.Stream.Position;
            }

            set
            {
                this.Stream.Position = value;
            }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            if (origin == SeekOrigin.End &&
                !this.entireRecordRead)
            {
                // The stream we have was created with the first 4K bytes of the record.  
                offset += this.recordSize;
                origin = SeekOrigin.Begin;
            }

            return this.Stream.Seek(offset, origin);
        }

        public override void SetLength(long value)
        {
            this.stream.SetLength(value);
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            if (!this.stream.CanRead)
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
            if (!this.stream.CanRead)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ObjectDisposed());
            }
            else
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
            }
        }

    }
}
