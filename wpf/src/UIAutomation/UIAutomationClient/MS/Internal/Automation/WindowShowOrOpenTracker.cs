//---------------------------------------------------------------------------
//
// <copyright file="WindowShowOrOpenTracker.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Class used to track new UI appearing and make sure any events
// are propogated to that new UI.
//
// History:  
//  06/17/2003 : BrendanM Ported to WCP
//  11/20/2003 : Micw renamed NewUITracker.cs to WindowShowOrOpenTracker.cs
//
//---------------------------------------------------------------------------

using System;
using System.Globalization;
using System.Text;
using System.Windows.Automation;
using System.Diagnostics;
using MS.Win32;

namespace MS.Internal.Automation
{
    // WindowShowOrOpenTracker - Class used to track new UI appearing and make sure any events
    // are propogated to that new UI.
    internal delegate void WindowShowOrOpenHandler( IntPtr hwnd, AutomationElement rawEl );

    // Class used to track new UI appearing and make sure any events
    // are propogated to that new UI.
    internal class WindowShowOrOpenTracker : WinEventWrap
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
 
        #region Constructors

        internal WindowShowOrOpenTracker(WindowShowOrOpenHandler newUIHandler)
            : base(new int[]
            {NativeMethods.EventObjectUIFragmentCreate, NativeMethods.EVENT_OBJECT_CREATE, NativeMethods.EVENT_OBJECT_SHOW}) 
        {
            AddCallback(newUIHandler);
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
            // ignore any event not pertaining directly to the window
            if (idObject != UnsafeNativeMethods.OBJID_WINDOW)
                return;

            // Ignore if this is a bogus hwnd (shouldn't happen)
            if (hwnd == IntPtr.Zero)
                return;

            NativeMethods.HWND nativeHwnd = NativeMethods.HWND.Cast( hwnd );

            // Ignore windows that have been destroyed
            if (! SafeNativeMethods.IsWindow( nativeHwnd ))
                return;

            // Ignore invisible windows (IsWindowVisible takes the hwnd ancestor 
            // visibility into account)
            if (! SafeNativeMethods.IsWindowVisible( nativeHwnd ))
                return; 

            if (eventId == NativeMethods.EVENT_OBJECT_CREATE)
            {
                // Ignore WCP hwnd creates; we get eventId EventObjectUIFragmentCreate for those
                // 
                string str = ProxyManager.GetClassName(nativeHwnd);
                if (String.Compare(str, 0, _wcpClassName, 0, _wcpClassName.Length, StringComparison.OrdinalIgnoreCase) == 0)
                    return;
            }

            AutomationElement rawEl = AutomationElement.FromHandle( hwnd );

            // Do the notify.  Note that this handler is used to notify client-side UIAutomation providers of windows
            // being destroyed or hidden.  The delegate called here is itself protected by a lock.  This delegate may
            // call out to proxies but also calls ClientEventManager.RaiseEventInThisClientOnly which properly
            // queues the actual callout to client code.
            object [] handlers = GetHandlers();
            Debug.Assert(handlers.Length <= 1, "handlers.Length");
            if (handlers.Length > 0)
                ((WindowShowOrOpenHandler)handlers[0])( hwnd, rawEl );
        }

        #endregion Internal Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
 
        #region Private Fields

        static readonly string _wcpClassName = "HwndWrapper";

        #endregion Private Fields
    }
}
