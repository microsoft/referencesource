//---------------------------------------------------------------------------
//
// <copyright file="AutomationPatternInfo.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: class containing information about an automation property
//
// History:  
//  06/04/2003 : BrendanM Ported to WCP
//
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;
using System.Windows.Automation;
using System.Windows.Automation.Peers;

namespace MS.Internal.Automation
{
    // struct containing information about an automation pattern
    internal delegate object WrapObject( AutomationPeer peer, object iface, IntPtr hwnd );

    internal class AutomationPatternInfo
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        internal AutomationPatternInfo( 
            AutomationPattern id,
            WrapObject wcpWrapper)
        {
            _id = id;
            _wcpWrapper = wcpWrapper;
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------
 
        #region Internal Properties

        internal AutomationPattern         ID                    { get { return _id; } }
        internal WrapObject                WcpWrapper            { get { return _wcpWrapper; } }


        #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        #region Private Fields

        private AutomationPattern _id;
        private WrapObject _wcpWrapper;

        #endregion Private Fields
    }
}
