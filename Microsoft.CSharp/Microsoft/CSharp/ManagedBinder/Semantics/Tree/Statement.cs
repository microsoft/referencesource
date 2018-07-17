// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal abstract class EXPRSTMT : EXPR
    {
        private EXPRSTMT NextStatement;
        public EXPRSTMT GetOptionalNextStatement()
        {
            return NextStatement;
        }
        public void SetOptionalNextStatement(EXPRSTMT nextStatement)
        {
            NextStatement = nextStatement;
        }
    }
}
