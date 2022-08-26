//---------------------------------------------------------------------------
//
// <copyright file="InheritedPropertyChangedEventArgs.cs" company="Microsoft">
//    Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Event args for the (internal) InheritedPropertyChanged event
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace MS.Internal
{
    // Event args for the (internal) InheritedPropertyChanged event
    internal class InheritedPropertyChangedEventArgs : EventArgs
    {
        internal InheritedPropertyChangedEventArgs(ref InheritablePropertyChangeInfo info)
        {
            _info = info;
        }

        internal InheritablePropertyChangeInfo Info
        {
            get { return _info; }
        }

        private InheritablePropertyChangeInfo _info;
    }

    // Handler delegate for the (internal) InheritedPropertyChanged event
    internal delegate void InheritedPropertyChangedEventHandler(object sender, InheritedPropertyChangedEventArgs e);
}
