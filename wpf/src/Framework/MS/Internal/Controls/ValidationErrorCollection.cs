//---------------------------------------------------------------------------
//
// <copyright file="validationerrorcollection.cs" company="Microsoft">
//    Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description:
//      ValidationErrorCollection contains the list of ValidationErrors from
//      the various Bindings and MultiBindings on an Element.  ValidationErrorCollection
//      be set through the Validation.ErrorsProperty.
//
// See specs at http://avalon/connecteddata/Specs/Validation.mht
//
// History:
//  5/3/2004       mharper: created.
//
//---------------------------------------------------------------------------


using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;


namespace MS.Internal.Controls
{
    /// <summary>
    ///      ValidationErrorCollection contains the list of ValidationErrors from
    ///      the various Bindings and MultiBindings on an Element.  ValidationErrorCollection
    ///      be set through the Validation.ErrorsProperty.
    /// </summary>
    internal class ValidationErrorCollection : ObservableCollection<ValidationError>
    {

        /// <summary>
        /// Empty collection that serves as a default value for
        /// Validation.ErrorsProperty.
        /// </summary>
        public static readonly ReadOnlyObservableCollection<ValidationError> Empty =
                new ReadOnlyObservableCollection<ValidationError>(new ValidationErrorCollection());
    }
}
