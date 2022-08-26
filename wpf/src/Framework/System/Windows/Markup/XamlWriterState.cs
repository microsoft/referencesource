//---------------------------------------------------------------------------
//
// File: XamlWriterState.cs
//
// Description:
//   Xaml Writer States for values that are of type Expression.
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Markup
{
    /// <summary>
    ///    Xaml Writer States are possible serialization events
    /// </summary>
    public enum XamlWriterState
    {
        /// <summary>
        ///    Serialization is starting
        /// </summary>
        Starting
            = 0,
        /// <summary>
        ///     Serialization is done
        /// </summary>
        Finished = 1
    }
}

