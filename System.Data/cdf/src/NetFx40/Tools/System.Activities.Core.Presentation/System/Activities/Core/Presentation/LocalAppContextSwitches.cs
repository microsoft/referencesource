//------------------------------------------------------------------------------
// <copyright file="LocalAppContextSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Activities.Core.Presentation
{
    internal static class LocalAppContextSwitches
    {
        public static bool UseLegacyAccessibilityFeatures
        {
            get
            {
                return System.Activities.Presentation.LocalAppContextSwitches.UseLegacyAccessibilityFeatures;
            }
        }
        
        public static bool UseLegacyAccessibilityFeatures2
        {
            get
            {
                return System.Activities.Presentation.LocalAppContextSwitches.UseLegacyAccessibilityFeatures2;
            }
        }
    }
}
