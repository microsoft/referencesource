// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.ComponentModel;

namespace Microsoft.CSharp.RuntimeBinder
{
    /// <summary>
    /// Represents information about a <see cref="CSharpInvokeBinder" /> or <see cref="CSharpInvokeMemberBinder" /> that
    /// is not specific to any particular argument given to those operations.
    /// </summary>
    internal enum CSharpCallFlags
    {
        /// <summary>
        /// No extra information.
        /// </summary>
        None = 0,

        /// <summary>
        /// The method was called given only a simple name, such as M(), unlike x.M().
        /// </summary>
        SimpleNameCall = 1,

        /// <summary>
        /// The call is permitted to bind against special names
        /// </summary>
        EventHookup = 2,
        
        ResultDiscarded = 4,
    }
}
