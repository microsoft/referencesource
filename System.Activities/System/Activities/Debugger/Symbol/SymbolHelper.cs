//-----------------------------------------------------------------------
// <copyright file="SymbolHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------
namespace System.Activities.Debugger.Symbol
{
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Linq;
    using System.Runtime;
    using System.Security;
    using System.Security.Cryptography;

    internal static class SymbolHelper
    {
        // These Guid values come from the VS source - VSCodeDebugAdpapterHost - src\product\VSCodeDebuggerHost\AD7\Definition\AD7Guids.cs
        static readonly Guid Md5IdentifierGuid = new Guid("406ea660-64cf-4c82-b6f0-42d48172a799");
        static readonly int Md5HashLength = 16;
        static readonly Guid Sha1IdentifierGuid = new Guid("ff1816ec-aa5e-4d10-87f7-6f4963833460");
        static readonly int Sha1HashLength = 20;
        static readonly Guid Sha256IdentifierGuid = new Guid("8829d00f-11b8-4213-878b-770e8597ac16");
        static readonly int Sha256HashLength = 32;

        public static Guid ChecksumProviderId
        {
            get
            {
                if (LocalAppContextSwitches.UseMD5ForWFDebugger)
                {
                    return Md5IdentifierGuid;
                }
                else if (LocalAppContextSwitches.UseSHA1HashForDebuggerSymbols)
                {
                    return Sha1IdentifierGuid;
                }
                else
                {
                    return Sha256IdentifierGuid;
                }
            }
        }

        // This is the same Encode/Decode logic as the WCF FramingEncoder
        public static int ReadEncodedInt32(BinaryReader reader)
        {
            int value = 0;
            int bytesConsumed = 0;
            while (true)
            {
                int next = reader.ReadByte();
                value |= (next & 0x7F) << (bytesConsumed * 7);
                bytesConsumed++;
                if ((next & 0x80) == 0)
                {
                    break;
                }
            }

            return value;
        }

        // This is the same Encode/Decode logic as the WCF FramingEncoder
        public static void WriteEncodedInt32(BinaryWriter writer, int value)
        {
            Fx.Assert(value >= 0, "Must be non-negative");

            while ((value & 0xFFFFFF80) != 0)
            {
                writer.Write((byte)((value & 0x7F) | 0x80));
                value >>= 7;
            }

            writer.Write((byte)value);
        }

        public static int GetEncodedSize(int value)
        {
            Fx.Assert(value >= 0, "Must be non-negative");

            int count = 1;
            while ((value & 0xFFFFFF80) != 0)
            {
                count++;
                value >>= 7;
            }

            return count;
        }

        public static byte[] CalculateChecksum(string fileName)
        {
            Fx.Assert(!string.IsNullOrEmpty(fileName), "fileName should not be empty or null");
            byte[] checksum;
            try
            {
                using (StreamReader streamReader = new StreamReader(fileName))
                {
                    using (HashAlgorithm hashAlgorithm = CreateHashProvider())
                    {
                        checksum = hashAlgorithm.ComputeHash(streamReader.BaseStream);
                    }
                }
            }
            catch (IOException)
            {
                // DirectoryNotFoundException and FileNotFoundException are expected
                checksum = null;
            }
            catch (UnauthorizedAccessException)
            {
                // UnauthorizedAccessException is expected
                checksum = null;
            }
            catch (SecurityException)
            {
                // Must not have had enough permissions to access the file.
                checksum = null;
            }

            return checksum;
        }

        [Fx.Tag.SecurityNote(Critical = "Used to get a string from checksum that is provided by the user/from a file.",
            Safe = "We not exposing any critical data. Just converting the byte array to a hex string.")]
        [SecuritySafeCritical]
        public static string GetHexStringFromChecksum(byte[] checksum)
        {
            return checksum == null ? string.Empty : string.Join(string.Empty, checksum.Select(x => x.ToString("X2")).ToArray());
        }

        [Fx.Tag.SecurityNote(Critical = "Used to validate checksum that is provided by the user/from a file.",
            Safe = "We not exposing any critical data. Just validating that the provided checksum meets the format for the checksums we produce.")]
        [SecuritySafeCritical]
        internal static bool ValidateChecksum(byte[] checksumToValidate)
        {
            // We are using MD5.ComputeHash, which will return a 16 byte array.
            if (LocalAppContextSwitches.UseMD5ForWFDebugger)
            {
                return checksumToValidate.Length == Md5HashLength;
            }
            else if (LocalAppContextSwitches.UseSHA1HashForDebuggerSymbols)
            {
                return checksumToValidate.Length == Sha1HashLength;
            }
            else
            {
                return checksumToValidate.Length == Sha256HashLength;
            }
        }

        [SuppressMessage("Microsoft.Cryptographic.Standard", "CA5350:MD5CannotBeUsed",
            Justification = "Design has been approved.  We are not using MD5 for any security or cryptography purposes but rather as a hash.")]
        [SuppressMessage("Microsoft.Cryptographic.Standard", "CA5354:SHA1CannotBeUsed",
            Justification = "Design has been approved.  We are not using SHA1 for any security or cryptography purposes but rather as a hash.")]
        static HashAlgorithm CreateHashProvider()
        {
            if (LocalAppContextSwitches.UseMD5ForWFDebugger)
            {
                return new MD5CryptoServiceProvider();
            }
            else if (LocalAppContextSwitches.UseSHA1HashForDebuggerSymbols)
            {
                return new SHA1CryptoServiceProvider();
            }
            else
            {
                return new SHA256CryptoServiceProvider();
            }
        }
    }
}
