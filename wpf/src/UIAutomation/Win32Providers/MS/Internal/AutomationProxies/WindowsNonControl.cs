//---------------------------------------------------------------------------
//
// <copyright file="WindowsNonControl.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: A Windows Proxy to set IsContent and IsControl to false.
//  By setting both IsContent and IsControl to false this will hide these
//  controls from the Content view of the Automation Tree.
//
// History:  
//  02/24/2005 : Microsoft created
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Provider;
using MS.Win32;

namespace MS.Internal.AutomationProxies
{
    class WindowsNonControl: ProxyHwnd
    {
        // ------------------------------------------------------
        //
        // Constructors
        //
        // ------------------------------------------------------

        #region Constructors

        WindowsNonControl(IntPtr hwnd, ProxyFragment parent, int item)
            : base(hwnd, parent, item)
        {
            _fIsContent = false;
        }

        #endregion

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

            return new WindowsNonControl(hwnd, null, idChild);
        }

        #endregion

        //------------------------------------------------------
        //
        //  Patterns Implementation
        //
        //------------------------------------------------------

        #region ProxySimple Interface

        // Process all the Logical and Raw Element Properties
        internal override object GetElementProperty (AutomationProperty idProp)
        {
            if (idProp == AutomationElement.IsControlElementProperty)
            {
                return false;
            }

            return base.GetElementProperty (idProp);
        }

        #endregion
    }
}

