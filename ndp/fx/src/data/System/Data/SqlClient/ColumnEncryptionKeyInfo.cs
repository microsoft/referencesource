//------------------------------------------------------------------------------
// <copyright file="SqlCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">nivithla</owner>
// <owner current="true" primary="false">nivithla</owner>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Data.SqlClient {

    /// <summary>
    /// Class encapsulating Column encryption key info
    /// </summary>
    internal class ColumnEncryptionKeyInfo {
        internal readonly int KeyId;
        internal readonly int DatabaseId;
        internal readonly byte[] DecryptedKeyBytes;
        internal readonly byte[] KeyIdBytes;
        internal readonly byte[] DatabaseIdBytes;
        internal readonly byte[] KeyMetadataVersionBytes;

        private static readonly string _decryptedKeyName = "DecryptedKey";
        private static readonly string _keyMetadataVersionName = "KeyMetadataVersion";
        private static readonly string _className = "ColumnEncryptionKeyInfo";
        private static readonly string _bytePackageName = "BytePackage";
        private static readonly string _serializeToBufferMethodName = "SerializeToBuffer";
        private static readonly string _startOffsetName="StartOffset";

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="decryptedKey">Decrypted key bytes</param>
        /// <param name="databaseId">database id for this column encryption key</param>
        /// <param name="keyMetadataVersion">key metadata version for this column encryption key</param>
        /// <param name="keyid">key id for this column encryption key</param>
        internal ColumnEncryptionKeyInfo(byte[] decryptedKey, int databaseId, byte[] keyMetadataVersion, int keyid) {

            if (null == decryptedKey) { throw SQL.NullArgumentInConstructorInternal(_decryptedKeyName, _className); }
            if (0 == decryptedKey.Length) { throw SQL.EmptyArgumentInConstructorInternal(_decryptedKeyName, _className); }
            if (null == keyMetadataVersion) { throw SQL.NullArgumentInConstructorInternal(_keyMetadataVersionName, _className); }
            if (0 == keyMetadataVersion.Length) { throw SQL.EmptyArgumentInConstructorInternal(_keyMetadataVersionName, _className); }

            KeyId = keyid;
            DatabaseId = databaseId;
            DecryptedKeyBytes = decryptedKey;
            KeyMetadataVersionBytes = keyMetadataVersion;

            //Covert keyId to Bytes
            ushort keyIdUShort;

            try {
                keyIdUShort = (ushort)keyid;
            } catch (Exception e) {
                throw SQL.InvalidKeyIdUnableToCastToUnsignedShort(keyid, e);
            }

            KeyIdBytes = BitConverter.GetBytes(keyIdUShort);

            //Covert databaseId to Bytes
            uint databaseIdUInt;

            try {
                databaseIdUInt = (uint)databaseId;
            } catch (Exception e) {
                throw SQL.InvalidDatabaseIdUnableToCastToUnsignedInt(databaseId, e);
            }

            DatabaseIdBytes = BitConverter.GetBytes(databaseIdUInt);
        }

        /// <summary>
        /// Calculates number of bytes required to serialize this object
        /// </summary>
        /// <returns>Number of bytes required for serialization</returns>
        internal int GetLengthForSerialization() {
            int lengthForSerialization = 0;
            lengthForSerialization += DecryptedKeyBytes.Length;
            lengthForSerialization += KeyIdBytes.Length;
            lengthForSerialization += DatabaseIdBytes.Length;
            lengthForSerialization += KeyMetadataVersionBytes.Length;
            return lengthForSerialization;
        }

        /// <summary>
        /// Serialize this object in a given byte[] starting at a given offset
        /// </summary>
        /// <param name="bytePackage">byte array for serialization</param>
        /// <param name="startOffset">start offset in byte array</param>
        /// <returns>next available offset</returns>
        internal int SerializeToBuffer(byte[] bytePackage, int startOffset) {

            if(null == bytePackage) { throw SQL.NullArgumentInternal(_bytePackageName, _className, _serializeToBufferMethodName); }
            if(0==bytePackage.Length) { throw SQL.EmptyArgumentInternal(_bytePackageName, _className, _serializeToBufferMethodName); }
            if (!(startOffset < bytePackage.Length)) { throw SQL.OffsetOutOfBounds(_startOffsetName, _className, _serializeToBufferMethodName); }
            if ( (bytePackage.Length - startOffset) < GetLengthForSerialization() ) { throw SQL.InsufficientBuffer(_bytePackageName, _className, _serializeToBufferMethodName); }

            Buffer.BlockCopy(DatabaseIdBytes, 0, bytePackage, startOffset, DatabaseIdBytes.Length);
            startOffset += DatabaseIdBytes.Length;
            Buffer.BlockCopy(KeyMetadataVersionBytes, 0, bytePackage, startOffset, KeyMetadataVersionBytes.Length);
            startOffset += KeyMetadataVersionBytes.Length;
            Buffer.BlockCopy(KeyIdBytes, 0, bytePackage, startOffset, KeyIdBytes.Length);
            startOffset += KeyIdBytes.Length;
            Buffer.BlockCopy(DecryptedKeyBytes, 0, bytePackage, startOffset, DecryptedKeyBytes.Length);
            startOffset += DecryptedKeyBytes.Length;

            return startOffset;
        }
    }
}
