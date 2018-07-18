// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal enum NullableCallLiftKind
    {
        NotLifted,
        Operator,
        EqualityOperator,
        InequalityOperator,
        UserDefinedConversion,
        NullableConversion,
        NullableConversionConstructor,
        NullableIntermediateConversion,
        NotLiftedIntermediateConversion
    }
}
