//---------------------------------------------------------------------------
// File: CurrentWPFCulture.cs
//
// Description:
// Helper class that allows access to localized values based on the current WPF culture.
//
// Copyright (C) 2006 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------
using System.Diagnostics;
using System.Globalization;
using System.Windows;

using System;

namespace Microsoft.Windows.Themes
{
    /// <summary>
    /// Public class used to expose some properties of the culture
    /// the platform is localized to.
    /// </summary>
    public static class PlatformCulture
    {
        /// <summary>
        /// FlowDirection of the culture the platform is localized to.
        /// </summary>
        public static FlowDirection FlowDirection
        {
            get
            {
                if (_platformCulture == null)
                {
                    _platformCulture = MS.Internal.PlatformCulture.Value;
                }
                Debug.Assert(_platformCulture != null);

                return _platformCulture.TextInfo.IsRightToLeft ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
            }
        }

        private static CultureInfo _platformCulture;
    }
}
