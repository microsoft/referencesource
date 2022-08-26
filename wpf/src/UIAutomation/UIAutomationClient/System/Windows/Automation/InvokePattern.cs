//---------------------------------------------------------------------------
//
// <copyright file="InvokePattern" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Client-side wrapper for Invoke Pattern
//
// History:  
//  06/23/2003 : BrendanM Ported to WCP
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Automation.Provider;
using MS.Internal.Automation;

namespace System.Windows.Automation
{


    /// <summary>
    /// Represents objects that have a single, unambiguous, action associated with them.
    /// 
    /// Examples of UI that implments this includes:
    /// Push buttons
    /// Hyperlinks
    /// Menu items
    /// Radio buttons
    /// Check boxes
    /// </summary>
#if (INTERNAL_COMPILE)
    internal class InvokePattern: BasePattern
#else
    public class InvokePattern: BasePattern
#endif
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        private InvokePattern(AutomationElement el, SafePatternHandle hPattern)
            : base(el, hPattern)
        {
            _hPattern = hPattern;
        }

        #endregion Constructors


        //------------------------------------------------------
        //
        //  Public Constants / Readonly Fields
        //
        //------------------------------------------------------
 
        #region Public Constants and Readonly Fields

        /// <summary>Invokable pattern</summary>
        public static readonly AutomationPattern Pattern = InvokePatternIdentifiers.Pattern;

        /// <summary>Event ID: Invoked - event used to watch for Invokable pattern Invoked events</summary>
        public static readonly AutomationEvent InvokedEvent = InvokePatternIdentifiers.InvokedEvent;

        #endregion Public Constants and Readonly Fields


        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
 
        #region Public Methods

        /// <summary>
        /// Request that the control initiate its action.
        /// Should return immediately without blocking.
        /// There is no way to determine what happened, when it happend, or whether
        /// anything happened at all.
        /// </summary>
        /// 
        /// <outside_see conditional="false">
        /// This API does not work inside the secure execution environment.
        /// <exception cref="System.Security.Permissions.SecurityPermission"/>
        /// </outside_see>
        public void Invoke()
        {
            UiaCoreApi.InvokePattern_Invoke(_hPattern);
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------
 
        #region Public Properties

        // No properties

        #endregion Public Properties


        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
 
        #region Internal Methods

        internal static object Wrap(AutomationElement el, SafePatternHandle hPattern, bool cached)
        {
            return new InvokePattern(el, hPattern);
        }

        #endregion Internal Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        #region Private Fields

        private SafePatternHandle _hPattern;

        #endregion Private Fields
    }
}
