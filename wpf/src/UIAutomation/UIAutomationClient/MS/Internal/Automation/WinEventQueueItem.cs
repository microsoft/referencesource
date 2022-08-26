//---------------------------------------------------------------------------
//
// <copyright file="WinEventQueueItem.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Class to create a queue on its own thread.
//
// History:  
//  06/17/2003 : BrendanM Ported to WCP
//
//---------------------------------------------------------------------------

using System.Windows.Automation;
using System.Windows.Automation.Provider;
using System;
using System.Threading;
using System.Collections;
using MS.Internal.Automation;
using MS.Win32;

namespace MS.Internal.Automation
{
    // Worker class used to handle WinEvents
    internal class WinEventQueueItem : QueueItem
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        internal WinEventQueueItem(WinEventWrap winEventWrap, int state)
        {
            _winEventWrap = winEventWrap;
            _state = state;
        }

        #endregion Constructors


        //------------------------------------------------------
        //
        //  Public Constants / Readonly Fields
        //
        //------------------------------------------------------
 
        #region Public Constants and Readonly Fields

        internal const int StartListening = 1;
        internal const int StopListening = 2;

        #endregion Public Constants and Readonly Fields


        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
 
        #region Internal Methods

        internal override void Process()
        {
            if (_state == StartListening)
            {
                _winEventWrap.StartListening();
            }
            else
            {
                _winEventWrap.StopListening();
            }
        }

        #endregion Internal Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        #region Private Fields

        private WinEventWrap _winEventWrap;
        private int _state;

        #endregion Private Fields
    }
}
