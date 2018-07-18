// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal enum TypeKind
    {
        TK_AggregateType,
        TK_VoidType,
        TK_NullType,
        TK_OpenTypePlaceholderType,
        TK_BoundLambdaType,
        TK_UnboundLambdaType,
        TK_MethodGroupType,
        TK_ErrorType,
        TK_NaturalIntegerType,
        TK_ArgumentListType,
        TK_ArrayType,
        TK_PointerType,
        TK_ParameterModifierType,
        TK_NullableType,
        TK_TypeParameterType
    }
}
