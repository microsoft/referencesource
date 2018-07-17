// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    // This class encapsulates the results of member lookup, allowing the consumers
    // to get at the inaccessible symbols, bogus symbols, and validly bound symbols.
    // ----------------------------------------------------------------------------

    internal partial class CMemberLookupResults
    {
        public TypeArray ContainingTypes { get; private set; }// Types that contain the member we're looking for.

        private Name m_pName; // The name that we're looking for.

        public CMemberLookupResults()
        {
            m_pName = null;
            ContainingTypes = null;
        }

        public CMemberLookupResults(
                TypeArray containingTypes,
                Name name)
        {
            m_pName = name;
            ContainingTypes = containingTypes;
            if (ContainingTypes == null)
            {
                ContainingTypes = BSYMMGR.EmptyTypeArray();
            }
        }

        public CMethodIterator GetMethodIterator(// TODO: Temporary until we move extension method reporting to a later time.
            CSemanticChecker pChecker, SymbolLoader pSymLoader, CType pObject, CType pQualifyingType, Declaration pContext, bool allowBogusAndInaccessible, bool allowExtensionMethods, int arity, EXPRFLAG flags, symbmask_t mask)
        {
            Debug.Assert(pSymLoader != null);
            CMethodIterator iterator = new CMethodIterator(pChecker, pSymLoader, m_pName, ContainingTypes, pObject, pQualifyingType, pContext, allowBogusAndInaccessible, allowExtensionMethods, arity, flags, mask);
            return iterator;
        }

        public partial class CMethodIterator
        {
        }
    }
}
