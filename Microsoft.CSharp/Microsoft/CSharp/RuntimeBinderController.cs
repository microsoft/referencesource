// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using Microsoft.CSharp.RuntimeBinder.Errors;

namespace Microsoft.CSharp.RuntimeBinder
{
    /////////////////////////////////////////////////////////////////////////////////
    // This class merely wraps a controller and throws a runtime binder exception
    // whenever we get an error during binding.

    internal class RuntimeBinderController : CController
    {
        public override void SubmitError(CError pError)
        {
            throw new RuntimeBinderException(pError.Text);
        }
    }
}
