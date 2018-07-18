// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;

namespace Microsoft.CSharp.RuntimeBinder
{
    internal interface ICSharpInvokeOrInvokeMemberBinder
    {
        // Helper methods.
        bool StaticCall { get; }
        bool ResultDiscarded { get; }

        // Members.
        Type CallingContext { get; }
        CSharpCallFlags Flags { get; }
        string Name { get; }
        IList<Type> TypeArguments { get; }
        IList<CSharpArgumentInfo> ArgumentInfo { get; }
    }
}