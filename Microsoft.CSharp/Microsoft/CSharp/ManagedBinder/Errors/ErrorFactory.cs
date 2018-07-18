// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Errors
{
    internal class CErrorFactory
    {
        public CError CreateError(ErrorCode iErrorIndex, params string[] args)
        {
            CError output = new CError();
            output.Initialize(iErrorIndex, args);
            return output;
        }
    }
}
