//------------------------------------------------------------------------------
// <copyright file="LocalAppContextSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches {
        private static int dontSupportPngFramesInIcons;
        private static int optimizePrintPreview;
        private static int doNotRemoveGdiFontsResourcesFromFontCollection;

        public static bool DontSupportPngFramesInIcons {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Drawing.DontSupportPngFramesInIcons", ref LocalAppContextSwitches.dontSupportPngFramesInIcons);
            }
        }

        public static bool OptimizePrintPreview {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Drawing.Printing.OptimizePrintPreview", ref LocalAppContextSwitches.optimizePrintPreview);
            }
        }

        public static bool DoNotRemoveGdiFontsResourcesFromFontCollection {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Drawing.Text.DoNotRemoveGdiFontsResourcesFromFontCollection", ref LocalAppContextSwitches.doNotRemoveGdiFontsResourcesFromFontCollection);
            }
        }
    }
}
