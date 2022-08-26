//---------------------------------------------------------------------------
//
// <copyright file="BindingValueChangedEventArgs.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: BindingValueChanged event arguments
//
// Specs:       http://avalon/connecteddata/M5%20Specs/UIBinding.mht
//
//---------------------------------------------------------------------------

using System;

namespace MS.Internal.Data
{
    /// <summary>
    /// Arguments for BindingValueChanged events.
    /// </summary>
    internal class BindingValueChangedEventArgs : EventArgs
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        internal BindingValueChangedEventArgs(object oldValue, object newValue) : base()
        {
            _oldValue = oldValue;
            _newValue = newValue;
        }

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        /// <summary>
        /// The old value of the binding.
        /// </summary>
        public object OldValue
        {
            get { return _oldValue; }
        }

        /// <summary>
        /// The new value of the binding.
        /// </summary>
        public object NewValue
        {
            get { return _newValue; }
        }

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        private object _oldValue, _newValue;
    }
}
