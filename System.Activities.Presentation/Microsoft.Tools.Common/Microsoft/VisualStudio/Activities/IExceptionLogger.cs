// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.VisualStudio.Activities
{
    using System;

    internal interface IExceptionLogger
    {
        void LogException(Exception exception);
    }
}
