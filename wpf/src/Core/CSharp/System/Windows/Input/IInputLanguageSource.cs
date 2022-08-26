//---------------------------------------------------------------------------
//
// <copyright file=IInputLanguageSource.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Define IInputLanguageSource interface.
//
// History:
//  07/30/2003 : yutakas - ported from dotnet tree.
//
//---------------------------------------------------------------------------
using System.Security.Permissions;
using System.Collections;
using System.Globalization;

using System;

namespace System.Windows.Input 
{
    /// <summary>
    ///     An interface for controlling the input language source.
    /// </summary>
    // We may need to public this interface for a custom dispather.
    public interface IInputLanguageSource 
    {
        /// <summary>
        ///     This access to the current input language.
        /// </summary>
        void Initialize();

        /// <summary>
        ///     This access to the current input language.
        /// </summary>
        void Uninitialize();

        /// <summary>
        ///     This access to the current input language.
        /// </summary>
        CultureInfo CurrentInputLanguage
        {
            get;
            set;
        }

        /// <summary>
        ///     This returns the list of available input languages.
        /// </summary>
        IEnumerable InputLanguageList
        {
            get;
        }
    }
}
