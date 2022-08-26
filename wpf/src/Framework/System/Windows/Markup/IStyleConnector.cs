//---------------------------------------------------------------------------
//
// File: IStyleConnector.cs
//
// Description:
//   Provides methods used internally by the StyleBamlReader
//
// Copyright (C) 2004 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace System.Windows.Markup
{
    /// <summary>
    /// Provides methods used internally by the StyleBamlReader
    /// on compiled content.
    /// </summary>
    public interface IStyleConnector
    {
        /// <summary>
        /// Called by the StyleBamlReader to attach events on EventSetters and
        /// Templates in compiled content.
        /// </summary>
        void Connect(int connectionId, object target);
    }
}
