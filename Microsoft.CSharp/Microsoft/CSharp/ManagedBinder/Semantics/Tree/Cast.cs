// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRCAST : EXPR
    {
        public EXPR Argument;
        public EXPR GetArgument() { return Argument; }
        public void SetArgument(EXPR expr) { Argument = expr; }
        public EXPRTYPEORNAMESPACE DestinationType;
        public EXPRTYPEORNAMESPACE GetDestinationType() { return DestinationType; }
        public void SetDestinationType(EXPRTYPEORNAMESPACE expr) { DestinationType = expr; }
        public bool IsBoxingCast() { return (flags & (EXPRFLAG.EXF_BOX | EXPRFLAG.EXF_FORCE_BOX)) != 0; }
    }
}
