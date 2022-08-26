//---------------------------------------------------------------------------
//
// <copyright file="IHostServices.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//      Internal interface used for setting up the browser hosting
//      environment
//
//
// History:
//  06/04/2003: kusumav:     Ported to WCP tree and moved to separate file
// 
//---------------------------------------------------------------------------

using System;

using System.Windows;
using System.Windows.Controls;
using System.Security;
using System.Security.Permissions;
namespace MS.Internal.AppModel
{
    // <summary>
    // Internal interface used to set up the hosting environment for browser hosting
    // </summary>
    internal interface IHostService
    {        
        // <summary>
        // The client window passed in to host 
        // Needed for non-Avalon host scenarios
        // when the winow needs re-positioning within the host's UI
        // </summary>
        RootBrowserWindowProxy RootBrowserWindowProxy
        {
            get;
        }

        // <summary>
        // get the HWND of the host
        // We use this to parent our first Window to this window.
        // </summary>

        /// <SecurityNote>
        ///     Critical: Sub classing this can let you expose windows and break functionality
        ///     that relies on this data being valid
        /// </SecurityNote>
        IntPtr HostWindowHandle
        {
            [SecurityCritical]
            [UIPermissionAttribute(SecurityAction.InheritanceDemand, Unrestricted = true)]
            get;
        }
    }
}
