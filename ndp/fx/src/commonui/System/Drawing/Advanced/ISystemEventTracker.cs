//------------------------------------------------------------------------------
// <copyright file="ISystemEventTracker.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Internal {

    using System.Diagnostics;

    using System;

    // See SystemColorTracker
    internal interface ISystemColorTracker {
        void OnSystemColorChanged();
    }
}

