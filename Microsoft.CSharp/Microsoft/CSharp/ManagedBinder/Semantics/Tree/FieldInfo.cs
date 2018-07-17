// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRFIELDINFO : EXPR
    {
        public FieldSymbol Field()
        {
            return field;
        }
        public AggregateType FieldType()
        {
            return fieldType;
        }
        public void Init(FieldSymbol f, AggregateType ft)
        {
            field = f;
            fieldType = ft;
        }
        private FieldSymbol field;
        private AggregateType fieldType;
    }
}
