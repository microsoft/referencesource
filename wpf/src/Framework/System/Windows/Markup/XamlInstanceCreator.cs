//---------------------------------------------------------------------------
//
// File: XamlInstanceCreator.cs
//
// Description:
//   A base class that allows storing parser records for later instantiation.
//
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Markup 
{
    /// <summary>
    ///     A base class that allows storing parser records for later instantiation.
    /// </summary>
    public abstract class XamlInstanceCreator
    {
        /// <summary>
        ///     Creates the object that this factory represents.
        /// </summary>
        /// <returns>The instantiated object.</returns>
        public abstract object CreateObject();

    }
}
