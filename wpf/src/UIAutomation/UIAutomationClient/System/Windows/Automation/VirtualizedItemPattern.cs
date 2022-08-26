//---------------------------------------------------------------------------
//
// <copyright file="VirtualizedItemPattern" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Client-side wrapper for VirtualizedItem Pattern
//
// History:  
//  10/21/2008 : Microsoft - created
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Automation.Provider;
using MS.Internal.Automation;
using System.Runtime.InteropServices;

namespace System.Windows.Automation
{
    /// <summary>
    /// Represents items inside containers which can be virtualized, this pattern can be used to realize them.
    /// </summary>
#if (INTERNAL_COMPILE)
    internal class VirtualizedItemPattern: BasePattern
#else
    public class VirtualizedItemPattern: BasePattern
#endif
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        private VirtualizedItemPattern(AutomationElement el, SafePatternHandle hPattern)
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

        /// <summary>VirtualizedItem pattern</summary>
        public static readonly AutomationPattern Pattern = VirtualizedItemPatternIdentifiers.Pattern;

        #endregion Public Constants and Readonly Fields


        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
 
        #region Public Methods

        /// <summary>
        /// Request that a placeholder element make itself fully available. Blocks
        /// until element is available, which could take time.
        /// Parent control may scroll as a side effect if the container needs to
        /// bring the item into view in order to devirtualize it.
        /// </summary>
       public void Realize()
        {
            UiaCoreApi.VirtualizedItemPattern_Realize(_hPattern);
        }

        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
 
        #region Internal Methods

        static internal object Wrap(AutomationElement el, SafePatternHandle hPattern, bool cached)
        {
            return new VirtualizedItemPattern(el, hPattern);
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

