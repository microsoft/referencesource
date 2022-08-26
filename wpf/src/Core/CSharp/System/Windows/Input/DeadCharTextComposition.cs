//---------------------------------------------------------------------------
//
// <copyright file=DeadCharTextComposition.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description: the DeadCharTextComposition class
//
// History:  
//  06/14/2004 : yutakas created
//
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Text;
using System.Windows.Threading;
using System.Windows;
using System.Security; 
using MS.Win32;
using Microsoft.Win32; // for RegistryKey class

namespace System.Windows.Input
{

    /// <summary>
    ///     the DeadCharTextComposition class is the composition object for Dead key scequence.
    /// </summary>
    internal sealed class DeadCharTextComposition : TextComposition
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        ///<SecurityNote> 
        ///     Critical - calls TextComposition:ctor which is critical ( as it stores InputManager). 
        ///</SecurityNote> 
        [SecurityCritical]
        internal DeadCharTextComposition(InputManager inputManager, IInputElement source, string text, TextCompositionAutoComplete autoComplete, InputDevice inputDevice) : base(inputManager, source, text, autoComplete, inputDevice)
        {
        }

        #endregion Constructors


        //------------------------------------------------------
        //
        //  internal Properties
        //
        //------------------------------------------------------

        internal bool Composed
        {
            get {return _composed;}
            set {_composed = value;}
        }

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        // If this is true, the text has been composed with actual char.
        private bool _composed;
    }
}
