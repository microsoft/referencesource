// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Errors
{
    // this interface is used to decouple the error reporting
    // implementation from the error detection source. 
    interface IErrorSink
    {
        void SubmitError(CParameterizedError error);
        int ErrorCount();
    }
}
