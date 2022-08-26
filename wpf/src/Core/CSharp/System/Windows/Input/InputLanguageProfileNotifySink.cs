//---------------------------------------------------------------------------
//
// <copyright file=InputLanguageProfileNotify.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: The source of the input language of the thread.
//
// History:
//  07/30/2003 : yutakas - ported from dotnet tree.
//
//---------------------------------------------------------------------------
using MS.Win32;
using System.Collections;
using System.Globalization;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Interop;
using System.Runtime.InteropServices;

using System;

namespace System.Windows.Input
{

    //------------------------------------------------------
    //
    //  InputLanguageProfileNotifySink
    //
    //------------------------------------------------------
 
    /// <summary>
    ///     This is an internal. This is an implementation of ITfLanguageProfileNotifySink.
    /// </summary>
    internal class InputLanguageProfileNotifySink : UnsafeNativeMethods.ITfLanguageProfileNotifySink
    {

        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        internal InputLanguageProfileNotifySink(InputLanguageSource target)
        {
            _target = target;
        }

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
 
        /// <summary>
        ///     OnLanguageChange call back of the interface.
        /// </summary>
        public void OnLanguageChange(short langid, out bool accept)
        {
            accept = _target.OnLanguageChange(langid);
        }
        /// <summary>
        ///     OnLanguageChanged call back of the interface.
        /// </summary>

        public void OnLanguageChanged()
        {
            _target.OnLanguageChanged();
        }

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        // the owner of this sink.
        private InputLanguageSource _target;
    }

}
