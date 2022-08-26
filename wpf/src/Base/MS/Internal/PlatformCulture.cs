//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:  PlatformCulture.cs    
//
//  Contents: Internal class that exposes the culture the platform is localized to.
//
//  Created:   5-19-2006  Rafael Ruiz (rruiz)
//
//------------------------------------------------------------------------

using System;
using System.Globalization;
using System.Windows;
using MS.Internal.WindowsBase;      

namespace MS.Internal
{
    /// <summary>
    /// Exposes the CultureInfo for the culture the platform is localized to.
    /// </summary>    
    [FriendAccessAllowed]
    internal static class PlatformCulture
    {
        /// <summary>
        /// Culture the platform is localized to.
        /// </summary>    
        public static CultureInfo Value
        {
            get 
            {
                // Get the UI Language from the string table
                string uiLanguage = SR.Get(SRID.WPF_UILanguage);
                Invariant.Assert(!string.IsNullOrEmpty(uiLanguage), "No UILanguage was specified in stringtable.");
    
                // Return the CultureInfo for this UI language.
                return new CultureInfo(uiLanguage);
            }
        }

    }
}
