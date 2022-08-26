//---------------------------------------------------------------------------
//
// <copyright file="SecurityMgrSite.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description:
//              The SecurityMgrSite is an implementation of Urlmon's IInternetSecurityMgrSite. 
//
//              It is primarily used to supply an hwnd to be modal to- when a ProcessUrlAction call
//              is required to show UI. 
// History:
//  03/08/05: marka     Created. 
//---------------------------------------------------------------------------

using System; 
using MS.Win32; 
using System.Runtime.InteropServices;
using System.Windows ; 
using System.Security; 
using MS.Internal.AppModel;

namespace MS.Internal 
{
    internal class SecurityMgrSite : NativeMethods.IInternetSecurityMgrSite
    {
        internal SecurityMgrSite()
        {

        }

        ///<SecurityNote> 
        ///     Critical calls unsecure handle. 
        ///     This function should only be called by Urlmon. 
        ///
        ///     Which is un-managed code - ergo also critical. 
        ///</SecurityNote> 
        [SecurityCritical] 
        public void GetWindow( /* [out] */ ref IntPtr phwnd)
        {   
            phwnd = IntPtr.Zero;
            
            if ( Application.Current != null )
            {   
                Window curWindow = Application.Current.MainWindow;

                Invariant.Assert( Application.Current.BrowserCallbackServices == null || ( curWindow is RootBrowserWindow )); 
                
                if (curWindow != null)
                {
                    phwnd = curWindow.CriticalHandle; 
                }
            }
        }

        public void EnableModeless( /* [in] */ bool fEnable)
        {

        }
    }
}
