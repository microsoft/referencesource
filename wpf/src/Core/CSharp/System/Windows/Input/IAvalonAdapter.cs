//---------------------------------------------------------------------------
//
// <copyright file="IAvalonAdapter.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Interface implemented by AvalonAdapter in WinFormsIntegration to provide KeyboardNavigation 
//              with information to determine how to process a Keytab coming from ElementHost.
//
//---------------------------------------------------------------------------

using MS.Internal.PresentationCore;
using System.Runtime.CompilerServices;

namespace System.Windows.Input
{
    /// <summary>
    ///     Interface for AvalonAdapter, provides an extended OnNoMoreTabStops from IKeyboardInputSite.
    /// </summary>
    internal interface IAvalonAdapter
    {
        bool OnNoMoreTabStops(TraversalRequest request, ref bool ShouldCycle);
    }

    /// <summary>
    ///     Implementation of IAvalonAdapter, we need this to prevent the linker from optimizing IAvalonAdapter away.
    /// </summary>
    internal class AvalonAdapterImpl : IAvalonAdapter
    {
        bool IAvalonAdapter.OnNoMoreTabStops(TraversalRequest request, ref bool ShouldCycle)
        {
            return false;
        }
    }
}



