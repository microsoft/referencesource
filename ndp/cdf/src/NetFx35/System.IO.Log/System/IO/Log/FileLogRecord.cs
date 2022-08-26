//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;

    internal class FileLogRecord : LogRecord
    {
        FileLogRecordStream stream;
        internal FileLogRecord(FileLogRecordStream stream)
        {
            this.stream = stream;
        }

        public override Stream Data
        {
            get { return this.stream; }
        }

        public override SequenceNumber Previous
        {
            get 
            {
                if (this.stream.Header.IsRestartArea)
                {
                    return SequenceNumber.Invalid;
                }
                else
                {
                    return this.stream.Header.PreviousLsn;
                }
            }
        }

        public override SequenceNumber SequenceNumber
        {
            get { return this.stream.RecordSequenceNumber; }
        }

        public override SequenceNumber User
        {
            get 
            {
                if (this.stream.Header.IsRestartArea)
                {
                    return SequenceNumber.Invalid;
                }
                else
                {
                    return this.stream.Header.NextUndoLsn;
                }
            }
        }

        public override void Dispose()
        {
            this.stream.Close();
        }
    }
}
