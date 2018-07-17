// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using System.Linq;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /////////////////////////////////////////////////////////////////////////////////
    // Encapsulates a type list, including its size and metadata token.

    class TypeArray
    {
        private CType[] items;

        public TypeArray(CType[] types)
        {
            items = types;
            if (items == null)
            {
                items = new CType[0];
            }
        }

        public int Size { get { return this.items.Length; } }
        public int size { get { return Size; } }

        public bool HasErrors() { return false; }
        public CType Item(int i) { return items[i]; }
        public TypeParameterType ItemAsTypeParameterType(int i) { return items[i].AsTypeParameterType(); }

        public CType[] ToArray() { return this.items.ToArray(); }

        [System.Runtime.CompilerServices.IndexerName("EyeTim")]
        public CType this[int i]
        {
            get { return this.items[i]; }
        }

        public int Count
        {
            get { return this.items.Length; }
        }

#if DEBUG
        public void AssertValid()
        {
            Debug.Assert(size >= 0);
            for (int i = 0; i < size; i++)
            {
                Debug.Assert(items[i] != null);
            }
        }
#endif

        public void CopyItems(int i, int c, CType[] dest)
        {
            for (int j = 0; j < c; ++j)
            {
                dest[j] = items[i + j];
            }
        }

    }
}
