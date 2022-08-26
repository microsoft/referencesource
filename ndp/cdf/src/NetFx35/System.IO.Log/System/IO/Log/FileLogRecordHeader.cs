//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
namespace System.IO.Log
{
    using System.IO;
    using System.Diagnostics;
    
    internal struct FileLogRecordHeader
    {
        [Flags]
        enum LowFlags : byte
        {
            RestartArea = 0x01
        }

        internal const int Size = 20;
        internal const byte MAJORVER = 1;
        internal const byte MINORVER = 0;
      
        const int MajorVersionOffset = 0;
        const int MinorVersionOffset = 1;
        const int LowFlagsOffset = 2;
        const int HighFlagsOffset = 3;
        const int PreviousLsnOffsetHigh = 4;
        const int NextUndoLsnOffsetHigh = 12;

        byte[] bits;
        
        internal FileLogRecordHeader(byte[] bits)
        {
            if (bits == null)
            {
                this.bits = new Byte[Size];
                this.bits[MajorVersionOffset] = MAJORVER;
                this.bits[MinorVersionOffset] = MINORVER;
            }
            else
            {
                if (bits.Length < Size)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
                }

                // if version not supported then ???
                if (bits[MajorVersionOffset] != MAJORVER)
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.IncompatibleVersion());                

                this.bits = bits;
            }
        }

        internal bool IsRestartArea
        {
            get
            {
                return (((LowFlags)bits[LowFlagsOffset] & LowFlags.RestartArea) != 0);
            }
            
            set
            {
                LowFlags flags = (LowFlags)bits[LowFlagsOffset];
                if (value)
                    flags |= LowFlags.RestartArea;
                else
                    flags &= ~LowFlags.RestartArea;
                bits[LowFlagsOffset] = (byte)(flags);
            }
        }

        internal SequenceNumber PreviousLsn
        {
            get
            {
                return new SequenceNumber(
                    BitConverter.ToUInt64(this.bits, PreviousLsnOffsetHigh));
            }

            set { SequenceNumber.WriteUInt64(value.High, this.bits, PreviousLsnOffsetHigh); }
        }
        
        internal SequenceNumber NextUndoLsn
        {
            get { return new SequenceNumber(BitConverter.ToUInt64(this.bits, NextUndoLsnOffsetHigh)); }

            set { SequenceNumber.WriteUInt64(value.High, this.bits, NextUndoLsnOffsetHigh); }
        }

        internal byte[] Bits
        {
            get { return this.bits; }
        }
    }
}
