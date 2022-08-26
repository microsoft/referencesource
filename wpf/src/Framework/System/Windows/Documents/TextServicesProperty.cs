//---------------------------------------------------------------------------
//
// <copyright file=TextServicesProperty.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: TextServicesProperty implementation.
//
// History:
//  08/01/2003 : yutakas - Ported from dotnet tree.
//
//---------------------------------------------------------------------------

using System.Runtime.InteropServices;
using System.Windows.Threading;
using System.Security;
using System.Security.Permissions;

using System.Collections;
using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Input;
using System.Windows.Documents;
using MS.Win32;

using System;

namespace System.Windows.Documents
{
    //------------------------------------------------------
    //
    //  TextServicesProperty class
    //
    //------------------------------------------------------

    /// <summary>
    ///   This is an internal.
    ///   This is a holder for Cicero properties.
    ///        - Reading String.
    ///        - Input Language.
    ///        - Display Attribute.
    ///
    ///   



    internal class TextServicesProperty
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        internal TextServicesProperty(TextStore textstore)
        {
            _textstore = textstore;
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------

        #region Internal Methods

        /// <summary>
        ///    Calback function for TextEditSink
        ///    we track all property change here.
        /// </summary>
        /// <SecurityNote>
        /// Critical - commits changes to edit buffer, might alow input spoofing
        /// </SecurityNote>
        [SecurityCritical]
        internal void OnEndEdit(
            UnsafeNativeMethods.ITfContext context, 
            int ecReadOnly,
            UnsafeNativeMethods.ITfEditRecord editRecord)
        {
            if (_propertyRanges == null)
            {
                _propertyRanges = new TextServicesDisplayAttributePropertyRanges(_textstore);
            }

            _propertyRanges.OnEndEdit(context, ecReadOnly, editRecord);
        }

        // Callback from TextStore.OnLayoutUpdated.
        // Updates composition display attribute adorner on-screen location.
        internal void OnLayoutUpdated()
        {
            TextServicesDisplayAttributePropertyRanges displayAttributes = _propertyRanges as TextServicesDisplayAttributePropertyRanges;

            if (displayAttributes != null)
            {
                displayAttributes.OnLayoutUpdated();
            }
        }

        #endregion Internal Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private TextServicesPropertyRanges _propertyRanges;

        private readonly TextStore _textstore;

        #endregion Private Fields
    }
}
