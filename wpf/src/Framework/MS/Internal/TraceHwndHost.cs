#define TRACE

//---------------------------------------------------------------------------
//
// <copyright file="TraceHwndHost.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Defines TraceHwndHost class, for providing debugging information
//              for HwndHosts
//
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Windows;
using System.Windows.Data;
using MS.Internal.Data;
using MS.Win32;

namespace MS.Internal
{
    /// <summary>
    /// Provides a central mechanism for providing debugging information
    /// to aid programmers in using HwndHost.
    /// Helpers are defined here.
    /// The rest of the class is generated; see also: AvTraceMessage.txt and genTraceStrings.pl
    /// </summary>
    internal static partial class TraceHwndHost
    {
        // ------------------------------------------------------------------
        // Constructors
        // ------------------------------------------------------------------

        static TraceHwndHost()
        {
            // This tells tracing that IsEnabled should be true if we're in the debugger,
            // even if the registry flag isn't turned on.  By default, IsEnabled is only
            // true if the registry is set.
            _avTrace.EnabledByDebugger = true;
        }
    }
}

