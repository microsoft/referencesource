// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

namespace System.Reflection.Metadata.Ecma335
{
    internal static class MetadataTokens
    {
        /// <summary>
        /// Maximum number of tables that can be present in Ecma335 metadata.
        /// </summary>
        public static readonly int TableCount = 64; // headers use 64bit table masks
        
        /// <summary>
        /// Returns the offset of metadata heap data that corresponds 
        /// to the specified <paramref name="handle"/>.
        /// </summary>
        /// <returns>
        /// An offset in the corresponding heap, or -1 if <paramref name="handle"/> can only be interpreted in a context of a specific <see cref="MetadataReader"/>.
        /// </returns>
        public static int GetHeapOffset(Handle handle)
        {
            if (!handle.IsHeapHandle)
            {
                Throw.HeapHandleRequired();
            }

            if (handle.IsVirtual)
            {
                return -1;
            }

            return handle.Offset;
        }

        /// <summary>
        /// Returns the metadata token of the specified <paramref name="handle"/>.
        /// </summary>
        /// <returns>
        /// Metadata token, or 0 if <paramref name="handle"/> can only be interpreted in a context of a specific <see cref="MetadataReader"/>.
        /// </returns>
        /// <exception cref="ArgumentException">
        /// Handle represents a metadata entity that doesn't have a token.
        /// A token can only be retrieved for a metadata table handle or a heap handle of type <see cref="HandleKind.UserString"/>.
        /// </exception>
        public static int GetToken(Handle handle)
        {
            if (!handle.IsEntityOrUserStringHandle)
            {
                Throw.EntityOrUserStringHandleRequired();
            }

            if (handle.IsVirtual)
            {
                return 0;
            }

            return handle.Token;
        }

        /// <summary>
        /// Gets the <see cref="TableIndex"/> of the table corresponding to the specified <see cref="HandleKind"/>.
        /// </summary>
        /// <param name="type">Handle type.</param>
        /// <param name="index">Table index.</param>
        /// <returns>True if the handle type corresponds to an Ecma335 or Portable PDB table, false otherwise.</returns>
        public static bool TryGetTableIndex(HandleKind type, out TableIndex index)
        {
            // We don't have a HandleKind for PropertyMap, EventMap, MethodSemantics, *Ptr, AssemblyOS, etc. tables, 
            // but one can get ahold of one by creating a handle from a token and getting its Kind.
            if ((int)type < TableCount && ((1UL << (int)type) & (ulong)TableMask.AllTables) != 0)
            {
                index = (TableIndex)type;
                return true;
            }

            index = 0;
            return false;
        }

        #region Handle Factories 

        /// <summary>
        /// Creates a handle from a token value.
        /// </summary>
        /// <exception cref="ArgumentException">
        /// <paramref name="token"/> is not a valid metadata token.
        /// It must encode a metadata table entity or an offset in <see cref="HandleKind.UserString"/> heap.
        /// </exception>
        public static Handle Handle(int token)
        {
            if (!TokenTypeIds.IsEntityOrUserStringToken(unchecked((uint)token)))
            {
                Throw.InvalidToken();
            }

            return Metadata.Handle.FromVToken((uint)token);
        }

        private static int ToRowId(int rowNumber)
        {
            return rowNumber & (int)TokenTypeIds.RIDMask;
        }

        public static MethodDefinitionHandle MethodDefinitionHandle(int rowNumber)
        {
            return Metadata.MethodDefinitionHandle.FromRowId(ToRowId(rowNumber));
        }

        // debug

        public static MethodDebugInformationHandle MethodDebugInformationHandle(int rowNumber)
        {
            return Metadata.MethodDebugInformationHandle.FromRowId(ToRowId(rowNumber));
        }
        
        #endregion
    }
}
