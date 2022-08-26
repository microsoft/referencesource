//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.IO;

    public abstract class LogRecord : IDisposable
    {
        public abstract Stream Data { get; }
        public abstract SequenceNumber Previous { get; }
        public abstract SequenceNumber SequenceNumber { get; }
        public abstract SequenceNumber User { get; }

        public abstract void Dispose();
    }
}
