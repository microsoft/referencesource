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
    /// <summary>Identities the state machine type for this method.</summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    public class StateMachineAttribute : Attribute
    {
        /// <summary>Initializes the attribute.</summary>
        /// <param name="stateMachineType">The type that implements the state machine.</param>
        public StateMachineAttribute(Type stateMachineType)
        {
            this.StateMachineType = stateMachineType;
        }

        /// <summary>Gets the type that implements the state machine.</summary>
        public Type StateMachineType { get; private set; }
    }
}
