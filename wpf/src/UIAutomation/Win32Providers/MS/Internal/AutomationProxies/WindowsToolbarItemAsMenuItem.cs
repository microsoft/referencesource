//---------------------------------------------------------------------------
//
// <copyright file="WindowsToolbarItemAsMenuItem.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Some applications implement menus with toolbars.  This proxy
//              will used the IAccessible to expose these toolbar items as
//              menu items.  This proxy is derived from ToolbarItem since 
//              the underlying control really is a toolbar and ToolbarItem
//              knows how to communicate with a toolbars to get information for
//              toolbar items already.
//
// History:
//  01/31/2005 : Microsoft Created
//---------------------------------------------------------------------------

using System;
using System.Windows.Automation;
using MS.Win32;

namespace MS.Internal.AutomationProxies
{
    class ToolbarItemAsMenuItem : ToolbarItem
    {
        // ------------------------------------------------------
        //
        // Constructors
        //
        // ------------------------------------------------------

        #region Constructors

        internal ToolbarItemAsMenuItem(IntPtr hwnd, ProxyFragment parent, int item, int idCommand, Accessible acc)
            : base(hwnd, parent, item, idCommand)
        {
            _acc = acc;

            // Set the control type based on the IAccessible role.
            AccessibleRole role = acc.Role;
            if (role == AccessibleRole.MenuItem)
            {
                _cControlType = ControlType.MenuItem;
            }
            else
            {
                System.Diagnostics.Debug.Assert(false, "Unexpected role " + role);

            }

            // MenuItems are by default KeyboardFocusable.
            _fIsKeyboardFocusable = true;
        }

        #endregion

        // ------------------------------------------------------
        //
        // Patterns Implementation
        //
        // ------------------------------------------------------

        #region ProxySimple Interface

        // Returns a pattern interface if supported.
        internal override object GetPatternProvider (AutomationPattern iid)
        {
            // Treate these toolbar items as menuitems and only support Invoke or Expand/Collapse patterns.
            // Invoke Pattern needs to be supported when the item has no children.  When the item does have
            // children it needs to support ExpandCollapse Pattern.

            // Check if button is a separator
            if (IsSeparator())
            {
                return null;
            }

            // Check if button is disabled
            if (Misc.ProxySendMessageInt(_hwnd, NativeMethods.TB_ISBUTTONENABLED, new IntPtr(_idCommand), IntPtr.Zero) == 0)
            {
                return null;
            }

            // Check if button is hidden
            if (Misc.ProxySendMessageInt(_hwnd, NativeMethods.TB_ISBUTTONHIDDEN, new IntPtr(_idCommand), IntPtr.Zero) != 0)
            {
                return null;
            }

            // 
            if (iid == InvokePattern.Pattern)
            {
                // button is enabled and not hidden and not a separator
                return this;
            }

            //

            return null;
        }

        // Process all the Element Properties
        internal override object GetElementProperty(AutomationProperty idProp)
        {
            if (idProp == AutomationElement.HasKeyboardFocusProperty)
            {
                return IsFocused();
            }

            return base.GetElementProperty(idProp);
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

