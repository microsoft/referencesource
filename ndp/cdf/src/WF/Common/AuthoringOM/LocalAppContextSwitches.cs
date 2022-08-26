// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System.Workflow.ComponentModel
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int useLegacyHashForXomlFileChecksum;
        
        public static bool UseLegacyHashForXomlFileChecksum
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                // This switch needs to be set in the MSBuild.Exe.Config that is used to build the project.
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Workflow.ComponentModel.UseLegacyHashForXomlFileChecksum", ref useLegacyHashForXomlFileChecksum);
            }
        }
    }
}
