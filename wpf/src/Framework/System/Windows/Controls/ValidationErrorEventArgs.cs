//---------------------------------------------------------------------------
//
// <copyright file="validationerroreventargs.cs" company="Microsoft">
//    Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: 
//      EventArgs for ValidationError event.
//
// See specs at http://avalon/connecteddata/Specs/Validation.mht
//
// History:
//  5/13/2004       mharper: created.
//
//---------------------------------------------------------------------------
using System;
using System.Windows;

using MS.Internal;

namespace System.Windows.Controls
{

    /// <summary> Describes if a validation error has been added or cleared
    /// </summary>
    public enum ValidationErrorEventAction
    {
        /// <summary>A new ValidationError has been detected.</summary>
        Added,
        /// <summary>An existing ValidationError has been cleared.</summary>
        Removed,
    }


    /// <summary>
    /// EventArgs for ValidationError event.
    /// </summary>
    public class ValidationErrorEventArgs : RoutedEventArgs
    {
        /// <summary>
        /// Constructor
        /// </summary>
        internal ValidationErrorEventArgs(ValidationError validationError, ValidationErrorEventAction action)
        {
            Invariant.Assert(validationError != null);
            
            RoutedEvent = Validation.ErrorEvent;
            _validationError = validationError;
            _action = action;
        }


        /// <summary>
        ///     The ValidationError that caused this ValidationErrorEvent to 
        ///     be raised.
        /// </summary>
        public ValidationError Error
        {
            get 
            {
                return _validationError;
            }
        }

        /// <summary>
        ///     Action indicates whether the <seealso cref="Error"/> is a new error
        ///     or a previous error that has now been cleared.
        /// </summary>
        public ValidationErrorEventAction Action
        {
            get 
            {
                return _action;
            }
        }

        
        /// <summary>
        ///     The mechanism used to call the type-specific handler on the
        ///     target.
        /// </summary>
        /// <param name="genericHandler">
        ///     The generic handler to call in a type-specific way.
        /// </param>
        /// <param name="genericTarget">
        ///     The target to call the handler on.
        /// </param>
        protected override void InvokeEventHandler(Delegate genericHandler, object genericTarget)
        {
            EventHandler<ValidationErrorEventArgs> handler = (EventHandler<ValidationErrorEventArgs>) genericHandler;
            
            handler(genericTarget, this);
        }


        private ValidationError _validationError;
        private ValidationErrorEventAction _action;
        
    }
}

