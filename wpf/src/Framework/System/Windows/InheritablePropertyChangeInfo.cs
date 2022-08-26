//---------------------------------------------------------------------------
//
// File: InheritablePropertyChangeInfo.cs
//
// Description:
//   This data-structure is used
//   1. As the data that is passed around by the DescendentsWalker
//      during an inheritable property change tree-walk.
//
// Copyright (C) by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows
{
    /// <summary>
    ///     This is the data that is passed through the DescendentsWalker
    ///     during an inheritable property change tree-walk.
    /// </summary>
    internal struct InheritablePropertyChangeInfo
    {
        #region Constructors

        internal InheritablePropertyChangeInfo(
            DependencyObject rootElement,
            DependencyProperty  property, 
            EffectiveValueEntry oldEntry, 
            EffectiveValueEntry newEntry)
        {
            _rootElement = rootElement;
            _property = property;
            _oldEntry = oldEntry;
            _newEntry = newEntry;
        }

        #endregion Constructors

        #region Properties

        internal DependencyObject RootElement
        {
            get { return _rootElement; }
        }

        internal DependencyProperty Property
        {
            get { return _property; }
        }

        internal EffectiveValueEntry OldEntry
        {
            get { return _oldEntry; }
        }

        internal EffectiveValueEntry NewEntry
        {
            get { return _newEntry; }
        }

        #endregion Properties

        #region Data

        private DependencyObject _rootElement;
        private DependencyProperty  _property;
        private EffectiveValueEntry _oldEntry;
        private EffectiveValueEntry _newEntry;

        #endregion Data
    }
}

