//---------------------------------------------------------------------------
//
// <copyright file="CollectionViewRegisteringEventArgs.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Arguments to the CollectionViewRegistering event (see BindingOperations).
//
// See spec at http://sharepoint/sites/WPF/Specs/Shared%20Documents/v4.5/Cross-thread%20Collections.docx
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Data
{
    public class CollectionViewRegisteringEventArgs : EventArgs
    {
        internal CollectionViewRegisteringEventArgs(CollectionView view)
        {
            _view = view;
        }

        public CollectionView CollectionView
        {
            get { return _view; }
        }

        CollectionView _view;
    }
}
