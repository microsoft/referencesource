// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;

namespace Microsoft.CSharp.RuntimeBinder.Syntax
{
    internal enum TokenKind : byte
    {
        ArgList,
        MakeRef,
        RefType,
        RefValue,
        As,
        Base,
        Checked,
        Explicit,
        False,
        Implicit,
        Is,
        Null,
        This,
        True,
        TypeOf,
        Unchecked,
        Void,

        Equal,
        PlusEqual,
        MinusEqual,
        SplatEqual,
        SlashEqual,
        PercentEqual,
        AndEqual,
        HatEqual,
        BarEqual,
        LeftShiftEqual,
        RightShiftEqual,
        Question,
        Colon,
        ColonColon,
        LogicalOr,
        LogicalAnd,
        Bar,
        Hat,
        Ampersand,
        EqualEqual,
        NotEqual,
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,
        LeftShift,
        RightShift,
        Plus,
        Minus,
        Splat,
        Slash,
        Percent,
        Tilde,
        Bang,
        PlusPlus,
        MinusMinus,
        Dot,
        QuestionQuestion,

        Unknown,
    }
}