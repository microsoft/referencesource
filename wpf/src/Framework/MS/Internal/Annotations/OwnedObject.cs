//-----------------------------------------------------------------------------
//
// <copyright file="OwnedObject.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//    IOwnedObject is an internal interface that identifies objects in the
//    Annotation Framework Object Model that can belong to only one parent.
//    This restriction is in place to prevent OM structures that cannot be
//    reproduced from a round-trip serialization/deserialization.
//
// History:
//  05/02/2005: rruiz:    Added new interface.
//-----------------------------------------------------------------------------
using System;

namespace MS.Internal.Annotations
{
    /// <summary>
    ///     Interface that identifies classes in the Annotation Framework Object
    ///     Model that can belong to only one parent.
    /// </summary>
    internal interface IOwnedObject
    {
        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        #region Public Properties

        /// <summary>
        ///     Sets/gets the ownership status of this object.
        /// </summary>
        bool Owned
        {
            get;
            set;
        }

        #endregion Public Properties

        //------------------------------------------------------
        //
        //  Public Operators
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
    }
}
