//---------------------------------------------------------------------------
//
// <copyright file="WindowsStartMeny.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Implementation of a provider for the Classic Start Menu
//
// History:
//  09/07/2004 : Microsoft         Created
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Provider;
using MS.Win32;

namespace MS.Internal.AutomationProxies
{

    class WindowsStartMenu : ProxyHwnd, IRawElementProviderSimple
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public WindowsStartMenu(IntPtr hwnd, ProxyHwnd parent, int item)
            : base( hwnd, parent, item)
        {
            _sAutomationId = "StartMenu";
        }
        #endregion Constructors

        #region Proxy Create

        // Static Create method called by UIAutomation to create this proxy.
        // returns null if unsuccessful 
        internal static IRawElementProviderSimple Create(IntPtr hwnd, int idChild, int idObject)
        {
            return Create(hwnd, idChild);
        }

        private static IRawElementProviderSimple Create(IntPtr hwnd, int idChild)
        {
            // Something is wrong if idChild is not zero 
            if (idChild != 0)
            {
                System.Diagnostics.Debug.Assert(idChild == 0, "Invalid Child Id, idChild != 0");
                throw new ArgumentOutOfRangeException("idChild", idChild, SR.Get(SRID.ShouldBeZero));
            }

            return new WindowsStartMenu(hwnd, null, 0);
        }

        #endregion

    }
   
}
