//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//--------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Runtime.CompilerServices
{
    /// <summary>Identities the async state machine type for this method.</summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    public sealed class AsyncStateMachineAttribute : StateMachineAttribute
    {
        /// <summary>Initializes the attribute.</summary>
        /// <param name="stateMachineType">The type that implements the state machine.</param>
        public AsyncStateMachineAttribute(Type stateMachineType) : base(stateMachineType) { }
    }
}
