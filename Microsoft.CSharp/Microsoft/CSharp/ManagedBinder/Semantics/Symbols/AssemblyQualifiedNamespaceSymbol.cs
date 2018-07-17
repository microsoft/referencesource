// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // AssemblyQualifiedNamespaceSymbol
    //
    // Parented by an NamespaceSymbol. Represents an NamespaceSymbol within an aid (assembly/alias id).
    // The name is a form of the aid.
    // ----------------------------------------------------------------------------

    class AssemblyQualifiedNamespaceSymbol : ParentSymbol, ITypeOrNamespace
    {

        // ----------------------------------------------------------------------------
        // AssemblyQualifiedNamespaceSymbol
        // ----------------------------------------------------------------------------

        public bool IsType()
        {
            return false;
        }

        public bool IsNamespace()
        {
            return true;
        }

        public AssemblyQualifiedNamespaceSymbol AsNamespace()
        {
            return this;
        }

        public CType AsType()
        {
            return null;
        }

        public NamespaceSymbol GetNS()
        {
            return parent.AsNamespaceSymbol();
        }
    }
}
