﻿// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.ComponentModel;

namespace Microsoft.CSharp.RuntimeBinder
{
    /// <summary>
    /// Represents the set of conversion kinds in C# for use with <see cref="CSharpConvertBinder" /> instances.
    /// Instances of this enum are generated by the C# compiler.
    /// </summary>
    internal enum CSharpConversionKind
    {
        /// <summary>
        /// Implicit conversion in C#.
        /// </summary>
        ImplicitConversion,

        /// <summary>
        /// Explicit conversion in C#.
        /// </summary>
        ExplicitConversion,

        /// <summary>
        /// Array creation conversion in C#.
        /// </summary>
        ArrayCreationConversion,
    }
}