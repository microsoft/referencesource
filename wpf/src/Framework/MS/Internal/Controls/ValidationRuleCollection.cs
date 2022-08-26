//---------------------------------------------------------------------------
//
// <copyright file="validationrulecollection.cs" company="Microsoft">
//    Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: 
//     ValidationRulesCollection is a collection of ValidationRule
//     instances on either a Binding or a MultiBinding.  Each of the rules
//     is checked for validity on update
//
// See specs at http://avalon/connecteddata/Specs/Validation.mht
//
// History:
//  5/3/2004       mharper: created.
//
//---------------------------------------------------------------------------


using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Controls;

namespace MS.Internal.Controls
{

    /// <summary>
    ///     ValidationRulesCollection is a collection of ValidationRule
    ///     instances on either a Binding or a MultiBinding.  Each of the rules
    ///     is checked for validity on update
    /// </summary>
    internal class ValidationRuleCollection : Collection<ValidationRule>
    {

    //------------------------------------------------------
    //
    //  Protected Methods
    //
    //------------------------------------------------------

    #region Protected Methods

    /// <summary>
    /// called by base class Collection&lt;T&gt; when an item is added to list;
    /// raises a CollectionChanged event to any listeners
    /// </summary>
    protected override void InsertItem(int index, ValidationRule item)
    {
        if (item == null)
            throw new ArgumentNullException("item");
        base.InsertItem(index, item);
    }

    /// <summary>
    /// called by base class Collection&lt;T&gt; when an item is added to list;
    /// raises a CollectionChanged event to any listeners
    /// </summary>
    protected override void SetItem(int index, ValidationRule item)
    {
        if (item == null)
            throw new ArgumentNullException("item");
        base.SetItem(index, item);
    }

    #endregion Protected Methods

    }
}

