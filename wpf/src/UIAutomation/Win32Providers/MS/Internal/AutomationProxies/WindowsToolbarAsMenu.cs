//---------------------------------------------------------------------------
//
// <copyright file="WindowsToolbarAsMenu.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Some applications implement menus with toolbars.  This proxy
//              will used the IAccessible to expose these toolbars as
//              menus.  This proxy is derived from WindowsToolbar since 
//              the underlying control really is a toolbar and WindowsToolbar
//              knows how to communicate with then underlying toolbar control
//              already.
//
// History:
//  01/31/2005 : Microsoft Created
//---------------------------------------------------------------------------

using System;
using System.Windows.Automation;
using MS.Win32;

namespace MS.Internal.AutomationProxies
{
    class WindowsToolbarAsMenu : WindowsToolbar
    {
        // ------------------------------------------------------
        //
        // Constructors
        //
        // ------------------------------------------------------

        #region Constructors

        internal WindowsToolbarAsMenu(IntPtr hwnd, ProxyFragment parent, int item, Accessible acc)
            : base( hwnd, parent, item )
        {
            _acc = acc;

            // Set the control type based on the IAccessible role.
            AccessibleRole role = acc.Role;

            if (role == AccessibleRole.MenuBar)
            {
                _cControlType = ControlType.MenuBar;
                _sAutomationId = "MenuBar"; // This string is a non-localizable string
            }
            else if (role == AccessibleRole.MenuPopup)
            {
                _cControlType = ControlType.Menu;
                _sAutomationId = "MenuPopup"; // This string is a non-localizable string
            }
            else
            {
                System.Diagnostics.Debug.Assert(false, "Unexpected role " + role);
            }
        }

        #endregion

        // ------------------------------------------------------
        //
        // Private Fields
        //
        // ------------------------------------------------------

        #region Private Fields

        Accessible _acc;

        #endregion

    }
}

