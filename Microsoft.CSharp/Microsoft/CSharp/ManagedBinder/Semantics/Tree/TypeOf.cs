// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRTYPEOF : EXPR
    {
        public EXPRTYPEORNAMESPACE SourceType;
        public EXPRTYPEORNAMESPACE GetSourceType() { return SourceType; }
        public void SetSourceType(EXPRTYPEORNAMESPACE value) { SourceType = value; }
    }
}
