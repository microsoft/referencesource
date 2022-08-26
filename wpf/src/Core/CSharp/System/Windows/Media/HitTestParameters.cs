//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2003
//
//  File:       HitTestParameters
//------------------------------------------------------------------------------

using System;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;

using System.Collections;
using System.Diagnostics;
using MS.Internal;

namespace System.Windows.Media 
{
    /// <summary>
    /// This is the base class for packing together parameters for a hit test pass.
    /// </summary>
    public abstract class HitTestParameters
    {
        // Prevent 3rd parties from extending this abstract base class.
        internal HitTestParameters() {}
    }
}

