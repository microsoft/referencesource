//---------------------------------------------------------------------------
//
// <copyright file="BoundingRectTracker.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Class used to send BoundingRect changes for hwnds
//
// History:  
//  06/17/2003 : BrendanM Ported to WCP
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Automation;
using System.Runtime.InteropServices;
using System.ComponentModel;
using MS.Win32;

namespace MS.Internal.Automation
{
    // BoundingRectTracker - Class used to send BoundingRect changes for hwnds
    internal class BoundingRectTracker : WinEventWrap
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        internal BoundingRectTracker() 
            : base(new int[]{NativeMethods.EVENT_OBJECT_LOCATIONCHANGE, NativeMethods.EVENT_OBJECT_HIDE}) 
        {
            // Intentionally not setting the callback for the base WinEventWrap since the WinEventProc override
            // in this class calls RaiseEventInThisClientOnly to actually raise the event to the client.
        }

        #endregion Constructors


        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
 
        #region Internal Methods

        internal override void WinEventProc(int eventId, IntPtr hwnd, int idObject, int idChild, uint eventTime)
        {
            // Filter... send an event for hwnd only
            if ( hwnd == IntPtr.Zero || idObject != UnsafeNativeMethods.OBJID_WINDOW )
                return;

            switch (eventId)
            {
                case NativeMethods.EVENT_OBJECT_HIDE:           OnHide(hwnd, idObject, idChild); break;
                case NativeMethods.EVENT_OBJECT_LOCATIONCHANGE: OnLocationChange(hwnd, idObject, idChild); break;
            } 
        }

        #endregion Internal Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------
 
        #region Private Methods

        private void OnHide(IntPtr hwnd, int idObject, int idChild)
        {
            // Clear last hwnd/rect variables (stop looking for dups)
            _lastHwnd = hwnd;
            _lastRect = _emptyRect;
        }

        private void OnLocationChange(IntPtr hwnd, int idObject, int idChild)
        {
            // Filter... send events for visible hwnds only
            if (!SafeNativeMethods.IsWindowVisible(NativeMethods.HWND.Cast( hwnd )))
                return;

            HandleBoundingRectChange(hwnd);
        }

        private void HandleBoundingRectChange(IntPtr hwnd)
        {
            NativeMethods.HWND nativeHwnd = NativeMethods.HWND.Cast( hwnd );
            NativeMethods.RECT rc32 = new NativeMethods.RECT(0,0,0,0);

            // if GetWindwRect fails, most likely the nativeHwnd is an invalid window, so just return.
            if (!Misc.GetWindowRect(nativeHwnd, out rc32))
            {
                return;
            }

            // Filter... avoid duplicate events
            if (hwnd == _lastHwnd && Compare( rc32, _lastRect ))
            {
                return;
            }

            AutomationElement rawEl = AutomationElement.FromHandle(hwnd);

            //
            // 





            AutomationPropertyChangedEventArgs e = new AutomationPropertyChangedEventArgs(
                                        AutomationElement.BoundingRectangleProperty, 
                                        Rect.Empty, 
                                        new Rect (rc32.left, rc32.top, rc32.right - rc32.left, rc32.bottom - rc32.top));

            // 





            ClientEventManager.RaiseEventInThisClientOnly(AutomationElement.AutomationPropertyChangedEvent, rawEl, e);

            // save the last hwnd/rect for filtering out duplicates
            _lastHwnd = hwnd;
            _lastRect = rc32;
        }

        // 
        private static bool Compare( NativeMethods.RECT rc1, NativeMethods.RECT rc2 )
        {
            return rc1.left == rc2.left
                && rc1.top == rc2.top
                && rc1.right == rc2.right
                && rc1.bottom == rc2.bottom;
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        #region Private Fields

        private static NativeMethods.RECT _emptyRect = new NativeMethods.RECT(0,0,0,0);

        private NativeMethods.RECT _lastRect;      // keep track of last location
        private IntPtr     _lastHwnd;      // and hwnd for dup checking
        
        #endregion Private Fields
    }
}

