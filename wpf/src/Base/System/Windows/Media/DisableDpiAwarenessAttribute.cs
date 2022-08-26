//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2006
//
//  File:      DisableDpiAwarenessAttribute.cs
//
//  By default, WPF application is Dpi-Aware when the UI layout is calculated.
//  But if in any case, an application wants to host WPF control and doesn't 
//  want to support Dpi aware,  the way to achieve it is to add below attribute
//  value in its application assembly.
//
//     [assembly:System.Windows.Media.DisableDpiAwareness]
// 
//  Created:   06/01/2006 WeibZ
//
//------------------------------------------------------------------------

using System;

namespace System.Windows.Media
{
    /// <summary>
    /// DisableDpiAwarenessAttribute tells to disable DpiAwareness in this 
    /// application for WPF UI elements.
    /// </summary>
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]
    public sealed class DisableDpiAwarenessAttribute: Attribute
    {
        /// <summary>
        /// Ctor of DisableDpiAwareness
        /// </summary>
        public DisableDpiAwarenessAttribute( )
        {
        }
    }
}
