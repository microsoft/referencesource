//------------------------------------------------------------------------------
// <copyright file="LocalAppContextSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Activities.Presentation
{
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        internal const string UseLegacyAccessibilityFeaturesSwitchName = @"Switch.UseLegacyAccessibilityFeatures";
        internal const string UseLegacyAccessibilityFeatures2SwitchName = @"Switch.UseLegacyAccessibilityFeatures.2";
        internal const string UseLegacyAccessibilityFeatures3SwitchName = @"Switch.UseLegacyAccessibilityFeatures.3";
        
        private static int useLegacyAccessibilityFeatures;
        private static int useLegacyAccessibilityFeatures2;
        private static int useLegacyAccessibilityFeatures3;

        public static bool UseLegacyAccessibilityFeatures
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.UseLegacyAccessibilityFeaturesSwitchName, ref useLegacyAccessibilityFeatures);
            }
        }
        
        public static bool UseLegacyAccessibilityFeatures2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.UseLegacyAccessibilityFeatures2SwitchName, ref useLegacyAccessibilityFeatures2);
            }
        }

        public static bool UseLegacyAccessibilityFeatures3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.UseLegacyAccessibilityFeatures3SwitchName, ref useLegacyAccessibilityFeatures3);
            }
        }
    }
}
