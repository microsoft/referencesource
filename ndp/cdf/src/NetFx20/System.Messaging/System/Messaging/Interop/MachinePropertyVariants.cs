//------------------------------------------------------------------------------
// <copyright file="MachinePropertyVariants.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop
{

    using System.Diagnostics;

    using System;

    internal class MachinePropertyVariants : MessagePropertyVariants
    {
        public MachinePropertyVariants()
            : base(5, NativeMethods.MACHINE_BASE + 1)
        {
        }
    }
}
