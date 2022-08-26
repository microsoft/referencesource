//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    public sealed class TailPinnedEventArgs : EventArgs
    {
        SequenceNumber sequenceNumber;

        public TailPinnedEventArgs(SequenceNumber sequenceNumber) 
        {
            this.sequenceNumber = sequenceNumber;
        }            
        
        public SequenceNumber TargetSequenceNumber
        {
            get
            {
                return this.sequenceNumber;
            }
        }
    }
}
