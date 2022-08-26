//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    public class SequenceFullException : Exception
    {
        public SequenceFullException()
            : base(SR.GetString(SR.SequenceFull))
        {
        }

        public SequenceFullException(string message)
            : base(message)
        {
        }

        public SequenceFullException(string message, Exception inner)
            : base(message, inner)
        {
        }

        protected SequenceFullException(SerializationInfo info,
                                        StreamingContext context)
            : base(info, context)
        {
        }
    }
}
