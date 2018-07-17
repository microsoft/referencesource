// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // VariableSymbol
    //
    // VariableSymbol - a symbol representing a variable. Specific subclasses are 
    // used - FieldSymbol for member variables, LocalVariableSymbol for local variables
    // and formal parameters, 
    // ----------------------------------------------------------------------------

    class VariableSymbol : Symbol
    {
        protected CType type;                       // CType of the field.
    }
}