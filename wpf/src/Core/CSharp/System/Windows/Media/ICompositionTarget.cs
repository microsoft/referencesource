//------------------------------------------------------------------------------
//
// <copyright file="ICompositionTarget.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//      Definition of the ICompositionTarget interface used to register
//      composition targets with the MediaContext.
//
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows.Media.Composition;

namespace System.Windows.Media
{
    /// <summary>
    /// With this interface we register CompositionTargets with the
    /// MediaContext.
    /// </summary>
    internal interface ICompositionTarget : IDisposable
    {
        void Render(bool inResize, DUCE.Channel channel);
        void AddRefOnChannel(DUCE.Channel channel, DUCE.Channel outOfBandChannel);
        void ReleaseOnChannel(DUCE.Channel channel, DUCE.Channel outOfBandChannel);
    }
}

