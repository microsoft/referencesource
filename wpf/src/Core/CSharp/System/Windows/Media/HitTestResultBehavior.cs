//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2003
//
//  File:       HitTestResultBehavior
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
    /// Enum controls behavior when a positive hit occurs during hit testing.
    /// </summary>
    public enum HitTestResultBehavior 
    {
        /// <summary>
        /// Stop any further hit testing and return.
        /// </summary>
        Stop,
        
        /// <summary>
        /// Continue hit testing against next visual.
        /// </summary>
        Continue
    };
    
    /// <summary>
    /// Delegate for hit tester to control returning of hit information on visual.
    /// </summary>
    public delegate HitTestResultBehavior HitTestResultCallback(HitTestResult result);
}

