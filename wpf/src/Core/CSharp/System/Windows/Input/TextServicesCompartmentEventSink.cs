//---------------------------------------------------------------------------
//
// <copyright file=TextServicesCompartmentEventSink.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description: Manages Text Services Compartment.
//
// History:  
//  07/30/2003 : yutakas - Ported from .net tree.
//
//---------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;
using System.Windows.Threading;

using System.Diagnostics;
using System.Collections;
using System.Security.Permissions;
using MS.Utility;
using MS.Win32;

namespace System.Windows.Input 
{
    //------------------------------------------------------
    //
    //  TextServicesCompartmentManager class
    //
    //------------------------------------------------------
 
    /// <summary>
    /// This is a class to have a real implement of ITfCompartmentEventSink.
    /// </summary>
    internal class TextServicesCompartmentEventSink : UnsafeNativeMethods.ITfCompartmentEventSink
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        internal TextServicesCompartmentEventSink(InputMethod inputmethod)
        {
            _inputmethod = inputmethod;
        }

        //------------------------------------------------------
        //
        //  Public Method
        //
        //------------------------------------------------------

        /// <summary>
        ///  This is OnChange method of ITfCompartmentEventSink internface.
        /// </summary> 
        public void OnChange(ref Guid rguid)
        {
            _inputmethod.OnChange(ref rguid);
        }

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
                
        #region Private Fields

        private InputMethod _inputmethod;

        #endregion Private Fields
    }
}

