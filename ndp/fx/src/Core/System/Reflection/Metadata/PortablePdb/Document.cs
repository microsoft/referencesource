// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Diagnostics;

namespace System.Reflection.Metadata
{
    /// <summary>
    /// Source document in debug metadata. 
    /// </summary>
    /// <remarks>
    /// See also https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md#document-table-0x30.
    /// </remarks>
    internal struct Document
    {
        private readonly MetadataReader _reader;

        // Workaround: JIT doesn't generate good code for nested structures, so use RowId.
        private readonly int _rowId;

        internal Document(MetadataReader reader, DocumentHandle handle)
        {
            Debug.Assert(reader != null);
            Debug.Assert(!handle.IsNil);

            _reader = reader;
            _rowId = handle.RowId;
        }

        private DocumentHandle Handle => DocumentHandle.FromRowId(_rowId);

        /// <summary>
        /// Returns Document Name Blob.
        /// </summary>
        public DocumentNameBlobHandle Name => _reader.DocumentTable.GetName(Handle);
    }
}
