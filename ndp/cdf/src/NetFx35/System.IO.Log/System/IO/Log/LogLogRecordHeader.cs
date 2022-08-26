//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;

    [Flags]
    internal enum LogLogRecordFlags : ushort
    {
        Padding = 0x0001
    }

    internal struct LogLogRecordHeader
    {
        public const int Size = 4;
        public const byte CurrentMajorVersion = 1;
        public const byte CurrentMinorVersion = 0;

        const int MajorVersionOffset = 0;
        const int MinorVersionOffset = 1;
        const int FlagsOffset = 2;

        byte[] bits;

        public LogLogRecordHeader(byte[] bits)
        {
            if (bits == null)
                bits = new byte[Size];

            if (bits.Length < Size)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
            }

            this.bits = bits;
        }

        public byte[] Bits
        {
            get { return bits; }
        }

        public byte MajorVersion
        {
            get { return bits[MajorVersionOffset]; }
            set { bits[MajorVersionOffset] = value; }
        }

        public byte MinorVersion
        {
            /* get { return bits[MinorVersionOffset]; } */
            set { bits[MinorVersionOffset] = value; }
        }

        public bool Padding
        {
            get
            {
                LogLogRecordFlags flags;
                flags = (LogLogRecordFlags)BitConverter.ToUInt16(bits, FlagsOffset);

                return (flags & LogLogRecordFlags.Padding) != 0;
            }
            set
            {
                LogLogRecordFlags flags;
                flags = (LogLogRecordFlags)BitConverter.ToUInt16(bits, FlagsOffset);

                if (value)
                    flags |= LogLogRecordFlags.Padding;
                else
                    flags &= ~LogLogRecordFlags.Padding;

                WriteUInt16((ushort)flags, bits, FlagsOffset);
            }
        }


        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // LogLogRecordHeader was defined to track the reservations for individual records whose size may be 
        // lesser than the actual reservation. CLFS doesn't track the actual amount of space consumed for a reservation 
        // and tracks only the total reserved space. If we reserve 1 record of 10 bytes, we must consume all the 
        // 10 bytes. For appending records with size less than the actual reserved amount IO.Log uses a header block with 
        // necessary padding to match the reservation size. 
        // Eg, if we reserves 1 record of 10 bytes, then writes 1 record of 5 bytes from reservation, 
        // we add 5 bytes of padding, so that we consume the 10 byte reservation completely.

        //
        //          1 Byte            1 Byte               2 Bytes                   n Padding bytes.
        // ---------------------------------------------------------------------------------------------
        // | Major Version | Minor Version |            Flags               | Variable length Padding (Length + zeros)   
        // |                                                                                  
        // ---------------------------------------------------------------------------------------------

        // EncodePaddingSize sets the padding length in header bits after the header fields.
        // Eg. The following output shows the values for each byte in the header for padding size from 0 to 6. 
        // The byte after the flag contains the size of the padding needed and followed by zeros. 
        //
        //   4 byte header + padding lenth + padding zeros
        // 1 0 0 0 
        // 1 0 1 0 1
        // 1 0 1 0 2 0
        // 1 0 1 0 3 0 0
        // 1 0 1 0 4 0 0 0
        // 1 0 1 0 5 0 0 0 0
        // 1 0 1 0 6 0 0 0 0 0

        public static void EncodePaddingSize(byte[] padding,
                                             int offset,
                                             int length)
        {
            int index = offset;
            int padSize = length;

            while (padSize != 0)
            {
                padding[index] = (byte)(padSize & 0x7F);
                padSize = padSize >> 7;
                if (padSize != 0)
                    padding[index] |= 0x80;

                index++;
            }
        }
        //
        // DecodePaddingSize returns the padding length in the byte array. 
        // The caller will then use it as an offset to get to the actual record. 
        //

        unsafe public static long DecodePaddingSize(byte* data,
                                                    long length)
        {
            uint padSize = 0;
            int shift = 0;

            while (length > 0)
            {
                padSize |= (uint)((*data & 0x7f) << shift);
                if (padSize > Int32.MaxValue)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.LogCorrupt());
                }

                shift += 7;

                if ((*data & 0x80) == 0)
                    break;

                data++;
                length--;
            }

            return (long)padSize;
        }

        static void WriteUInt16(ushort value, byte[] where, int offset)
        {
            where[offset + 0] = (byte)((value & 0x00FF) >> 0);
            where[offset + 1] = (byte)((value & 0xFF00) >> 8);
        }
    }
}
