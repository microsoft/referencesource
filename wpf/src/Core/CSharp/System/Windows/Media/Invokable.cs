//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2003
//
//  File:       Invokable.cs
//------------------------------------------------------------------------------

using System.Windows.Media;
using System;

// TODOTODO Make our own namespace for internal stuff.
namespace System.Windows.Media
{
    #region IInvokable

    /// <summary>
    /// Any class can implement this interface to get events from an external source
    /// Used in Unmanaged -> Managed event relaying
    /// </summary>
    internal interface IInvokable
    {
        void RaiseEvent(byte[] buffer, int cb);
    }

    #endregion
}
