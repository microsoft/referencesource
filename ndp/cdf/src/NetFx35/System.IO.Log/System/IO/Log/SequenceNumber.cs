//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    // Add property for sequence number size
    using System;
    using System.Collections.Generic;

    [Serializable]
    public struct SequenceNumber : IComparable<SequenceNumber>
    {
        ulong sequenceNumberHigh;
        ulong sequenceNumberLow;

        public SequenceNumber(byte[] sequenceNumber)
        {
            if (sequenceNumber == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentNull("sequenceNumber"));
            }

            if ((sequenceNumber.Length != 8) &&
                (sequenceNumber.Length != 16))
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.ArgumentInvalid(SR.Argument_SequenceNumberLength));
            }

            this.sequenceNumberHigh = BitConverter.ToUInt64(sequenceNumber, 0);
            if (sequenceNumber.Length > 8)
            {
                this.sequenceNumberLow = BitConverter.ToUInt64(sequenceNumber, 8);
            }
            else
            {
                this.sequenceNumberLow = 0;
            }
        }

        internal SequenceNumber(ulong value)
        {
            this.sequenceNumberHigh = value;

            // Ensure that SequenceNumber.Invalid round-trips.
            //
            if (value == UInt64.MaxValue)
            {
                this.sequenceNumberLow = UInt64.MaxValue;
            }
            else
            {
                this.sequenceNumberLow = 0;
            }
        }

        internal SequenceNumber(ulong high, ulong low)
        {
            this.sequenceNumberHigh = high;
            this.sequenceNumberLow = low;
        }

        internal ulong High { get { return this.sequenceNumberHigh; } }
        internal ulong Low { get { return this.sequenceNumberLow; } }

        public static SequenceNumber Invalid
        {
            get
            {
                return new SequenceNumber(UInt64.MaxValue, UInt64.MaxValue);
            }
        }

        public static bool operator !=(
            SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) != 0);
        }

        public static bool operator <(
            SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) < 0);
        }

        public static bool operator <=(SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) <= 0);
        }

        public static bool operator ==(
            SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) == 0);
        }

        public static bool operator >(
            SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) > 0);
        }

        public static bool operator >=(
            SequenceNumber c1,
            SequenceNumber c2)
        {
            return (c1.CompareTo(c2) >= 0);
        }

        public int CompareTo(SequenceNumber other)
        {
            int result = this.sequenceNumberHigh.CompareTo(other.sequenceNumberHigh);
            if (result == 0)
                result = this.sequenceNumberLow.CompareTo(other.sequenceNumberLow);

            return result;
        }

        public bool Equals(SequenceNumber other)
        {
            return (this.CompareTo(other) == 0);
        }

        public override bool Equals(object other)
        {
            if (!(other is SequenceNumber))
                return false;

            SequenceNumber otherSeq = (SequenceNumber)(other);
            return this.Equals(otherSeq);
        }

        public byte[] GetBytes()
        {
            byte[] bytes;

            if (this.sequenceNumberLow == 0)
            {
                bytes = new byte[8];
                WriteUInt64(this.sequenceNumberHigh, bytes, 0);
            }
            else
            {
                bytes = new byte[16];
                WriteUInt64(this.sequenceNumberHigh, bytes, 0);
                WriteUInt64(this.sequenceNumberLow, bytes, 8);
            }

            return bytes;
        }

        public override int GetHashCode()
        {
            return
                this.sequenceNumberHigh.GetHashCode() ^
                this.sequenceNumberLow.GetHashCode();
        }

        internal static void WriteUInt64(ulong value, byte[] bits, int offset)
        {
            bits[offset + 0] = (byte)((value >> 0) & 0xFF);
            bits[offset + 1] = (byte)((value >> 8) & 0xFF);
            bits[offset + 2] = (byte)((value >> 16) & 0xFF);
            bits[offset + 3] = (byte)((value >> 24) & 0xFF);
            bits[offset + 4] = (byte)((value >> 32) & 0xFF);
            bits[offset + 5] = (byte)((value >> 40) & 0xFF);
            bits[offset + 6] = (byte)((value >> 48) & 0xFF);
            bits[offset + 7] = (byte)((value >> 56) & 0xFF);
        }
    }
}
