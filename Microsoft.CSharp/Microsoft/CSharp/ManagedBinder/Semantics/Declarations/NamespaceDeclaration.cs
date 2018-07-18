// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // NamespaceDeclaration
    //
    // NamespaceDeclaration - a symbol representing a declaration
    // of a namspace in the source. 
    //
    // firstChild/firstChild->nextChild enumerates the 
    // NSDECLs and AGGDECLs declared within this declaration.
    //
    // parent is the containing namespace declaration.
    //
    // Bag() is the namespace corresponding to this declaration.
    //
    // DeclNext() is the next declaration for the same namespace.
    // ----------------------------------------------------------------------------

    class NamespaceDeclaration : Declaration
    {
        public NamespaceSymbol Bag()
        {
            return bag.AsNamespaceSymbol();
        }

        public NamespaceSymbol NameSpace()
        {
            return bag.AsNamespaceSymbol();
        }
    }
}
