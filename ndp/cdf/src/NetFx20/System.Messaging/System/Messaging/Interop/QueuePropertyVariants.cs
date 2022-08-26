//------------------------------------------------------------------------------
// <copyright file="QueuePropertyVariants.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop
{

    using System.Diagnostics;

    using System;

    internal class QueuePropertyVariants : MessagePropertyVariants
    {

        private const int MaxQueuePropertyIndex = 26;

        public QueuePropertyVariants()
            : base(MaxQueuePropertyIndex, NativeMethods.QUEUE_PROPID_BASE + 1)
        {
        }
    }
}
