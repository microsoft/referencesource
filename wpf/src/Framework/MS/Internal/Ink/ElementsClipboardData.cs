//---------------------------------------------------------------------------
//
// File: ElementsClipboardData.cs
//
// Description:
//      A base class which can convert the clipboard data from/to FrameworkElement array
//
// Features:
//
// History:
//  11/17/2004 waynezen:       Created 
//
// Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

namespace MS.Internal.Ink
{
    internal abstract class ElementsClipboardData : ClipboardData
    {
        //-------------------------------------------------------------------------------
        //
        // Constructors
        //
        //-------------------------------------------------------------------------------
        
        #region Constructors

        // The default constructor
        internal ElementsClipboardData() { }

        // The constructor which takes a FrameworkElement array.
        internal ElementsClipboardData(UIElement[] elements)
        {
            if ( elements != null )
            {
                ElementList = new List<UIElement>(elements);
            }
        }

        #endregion Constructors

        //-------------------------------------------------------------------------------
        //
        // Internal Properties
        //
        //-------------------------------------------------------------------------------

        #region Internal Properties

        // Gets the element array.
        internal List<UIElement> Elements
        {
            get
            {
                if ( ElementList != null )
                {
                    return _elementList;
                }
                else
                {
                    return new List<UIElement>();
                }
            }
        }

        #endregion Internal Properties

        //-------------------------------------------------------------------------------
        //
        // Protected Properties
        //
        //-------------------------------------------------------------------------------

        #region Protected Properties

        // Sets/Gets the internal array list
        protected List<UIElement> ElementList
        {
            get
            {
                return _elementList;
            }
            set
            {
                _elementList = value;
            }
        }

        #endregion Protected Properties

        //-------------------------------------------------------------------------------
        //
        // Private Fields
        //
        //-------------------------------------------------------------------------------

        #region Private Fields

        private List<UIElement> _elementList;

        #endregion Private Fields

    }
}
