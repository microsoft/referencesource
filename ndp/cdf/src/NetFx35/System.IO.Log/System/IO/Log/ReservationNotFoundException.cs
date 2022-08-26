//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    public class ReservationNotFoundException : ArgumentException
    {
        public ReservationNotFoundException()
            : base(SR.GetString(SR.Argument_ReservationNotFound))
        {
        }

        public ReservationNotFoundException(string message)
            : base(message)
        {
        }

        public ReservationNotFoundException(string message, Exception inner)
            : base(message, inner)
        {
        }

        protected ReservationNotFoundException(SerializationInfo info,
                                        StreamingContext context)
            : base(info, context)
        {
        }
    }
}
