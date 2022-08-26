// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
#pragma warning disable 0436 //Disable the type conflict warning for the types used by LocalAppContext framework due to InternalsVisibleTo for System.Workflow.ComponentModel (Quirking)
namespace System.Workflow.Runtime
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int useLegacyHashForWorkflowDefinitionDispenserCacheKey;
        private static int useLegacyHashForSqlTrackingCacheKey;

        public static bool UseLegacyHashForWorkflowDefinitionDispenserCacheKey
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Workflow.Runtime.UseLegacyHashForWorkflowDefinitionDispenserCacheKey", ref useLegacyHashForWorkflowDefinitionDispenserCacheKey);
            }
        }

        public static bool UseLegacyHashForSqlTrackingCacheKey
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Workflow.Runtime.UseLegacyHashForSqlTrackingCacheKey", ref useLegacyHashForSqlTrackingCacheKey);
            }
        }
    }
}
