// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

namespace System.Runtime.CompilerServices
{
    public static class RuntimeFeature
    {
        /// <summary>
        /// Name of the Portable PDB feature.
        /// </summary>
        public const string PortablePdb = nameof(PortablePdb);

        /// <summary>
        /// Checks whether a certain feature is supported by the Runtime.
        /// </summary>
        public static bool IsSupported(string feature)
        {
            // Features should be added as public const string fields in the same class.
            // Example: public const string FeatureName = nameof(FeatureName);

            switch (feature)
            {
                case nameof(PortablePdb):
                    // For back compat we opt-out of using Portable PDBs before 4.7.2. This will prevent searching for them
                    // on disk, loading them, and adding their source information to diagnostic stack traces. When we first
                    // implemented the feature in 4.7.1 RTM we regressed microbenchmark performance significantly and did not include
                    // this an opt-out. A later fix added this opt-out/quirk and brought the performance back much closer to the original
                    // pre 4.7.1 performance.
                    //
                    // Unforetunately the implementation of the IsSupported API for PortablePDB has not been accurate in all
                    // shipped runtime versions:
                    // 4.7.1 RTM - [GOOD] - portable pdb is enabled and this API returns true
                    // 4.7.1 ZDP - [BAD] - portable pdb is disabled but IsSupported returns true
                    // 4.7.2 -     [BAD] - portable pdb is quirked so it may or may not be enabled but IsSupported always returns true
                    // 4.7.3+ -    [GOOD] - portable pdb is quirked so it may or may not be enabled and IsSupported accurately accounts for the quirk
                    return !AppContextSwitches.IgnorePortablePDBsInStackTraces;
            }

            return false;
        }
    }
}
