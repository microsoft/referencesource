//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Provides access to the new item during the InitializingNewItem event.
    /// </summary>
    public class InitializingNewItemEventArgs : EventArgs
    {
        /// <summary>
        ///     Instantiates a new instance of this class.
        /// </summary>
        public InitializingNewItemEventArgs(object newItem)
        {
            _newItem = newItem;
        }

        /// <summary>
        ///     The new item.
        /// </summary>
        public object NewItem
        {
            get { return _newItem; }
        }

        private object _newItem;
    }
}
