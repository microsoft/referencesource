// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal enum CONSTRESKIND
    {
        ConstTrue,
        ConstFalse,
        ConstNotConst,
    }
    internal enum LambdaParams
    {
        FromDelegate,
        FromLambda,
        Error
    }
    internal enum TypeOrSimpleNameResolution
    {
        Unknown,
        CType,
        SimpleName
    }
    internal enum InitializerKind
    {
        CollectionInitializer,
        ObjectInitializer
    }

    internal enum ConstantStringConcatenation
    {
        NotAString,
        NotYetCalculated,
        Calculated
    }

    internal enum ForeachKind
    {
        Array,
        String,
        Enumerator
    }

}
