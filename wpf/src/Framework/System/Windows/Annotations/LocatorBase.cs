//-----------------------------------------------------------------------------
//
// <copyright file="ContentLocatorBase.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//     ContentLocatorBase represents an object that identifies a piece of data.  It 
//     can be an ordered list of ContentLocatorParts (in which case its a 
//     ContentLocator) or it can be a set of Locators (in which case its a 
//     ContentLocatorGroup).
//
//     Spec: http://team/sites/ag/Specifications/Simplifying%20Store%20Cache%20Model.doc
//
// History:
//  10/04/2002: rruiz:    Added header comment to ObjectModel.cs
//  07/03/2003: magedz:   Renamed Link, LinkSequence to LocatorPart and Locator 
//                        respectively.
//  05/31/2003: LGolding: Ported to WCP tree.
//  07/15/2003: rruiz:    Rewrote implementations to extend abstract classes
//                        instead of implement interfaces; got rid of obsolete
//                        classes; put each class in separate file.
//  12/09/2003: ssimova:  Added Id property
//  12/03/2003: ssimova:  Changed LocatorParts to Parts
//  06/20/2004: rruiz:    Stripped out most of the API and made this an abstract
//                        class which supports notifying its owner of changes. 
//                        New concrete classes were introduced for sets and
//                        sequences. 
//-----------------------------------------------------------------------------
using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Xml;

using MS.Internal.Annotations;

namespace System.Windows.Annotations
{
    /// <summary>
    ///     ContentLocatorBase represents an object that identifies a piece of data.
    ///     It can be an ordered list of ContentLocatorParts (in which case its a 
    ///     ContentLocator) or it can be a set of Locators (in which case its a ContentLocatorGroup).
    /// </summary>
    public abstract class ContentLocatorBase : INotifyPropertyChanged2, IOwnedObject
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        /// <summary>
        ///     Internal constructor.  This makes the abstract class
        ///     unsubclassable by third-parties, as desired.
        /// </summary>
        internal ContentLocatorBase()
        {
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        /// <summary>
        ///     Create a deep clone of this ContentLocatorBase.
        /// </summary>
        /// <returns>a deep clone of this ContentLocatorBase; never returns null</returns>
        public abstract object Clone();
       
        #endregion Public Methods

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
        //  Public Properties
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Internal Events
        //
        //------------------------------------------------------

        #region Public Events

        /// <summary>
        /// 
        /// </summary>
        event PropertyChangedEventHandler INotifyPropertyChanged.PropertyChanged
        {
            add{ _propertyChanged += value; }
            remove{ _propertyChanged -= value; }
        }

        #endregion Public Events
        
        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------

        #region Internal Methods

        /// <summary>
        ///     Notify the owner this ContentLocatorBase has changed.
        ///     This method should be protected so only subclasses
        ///     could call it but that would expose it in the
        ///     public API space so we keep it internal.
        /// </summary>
        internal void FireLocatorChanged(string name)
        {
            if (_propertyChanged != null)
            {
                _propertyChanged(this, new System.ComponentModel.PropertyChangedEventArgs(name));
            }
        }

        /// <summary>
        /// </summary>
        bool IOwnedObject.Owned
        {
            get
            {
                return _owned;
            }
            set
            {
                _owned = value;
            }
        }

        /// <summary>
        ///     Internal Merge method used by the LocatorManager as it builds up
        ///     Locators.  We don't expose these methods publicly because they
        ///     are of little use and are optimized for use by the LM (e.g., we 
        ///     know the arguments aren't owned by anyone and can be modified in 
        ///     place).
        /// </summary>
        /// <param name="other">the ContentLocatorBase to merge</param>
        /// <returns>the resulting ContentLocatorBase (may be the same object the method
        /// was called on for perf reasons)</returns>
        internal abstract ContentLocatorBase Merge(ContentLocatorBase other);

        #endregion Internal Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------        

        #region Private Fields

        /// <summary>
        /// </summary>
        private bool   _owned;

        /// <summary>
        /// 
        /// </summary>
        private event PropertyChangedEventHandler _propertyChanged;

        #endregion Private Fields
    }
}
