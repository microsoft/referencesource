// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
using System;
using System.Runtime.CompilerServices;

namespace System
{
    internal static class LocalAppContextSwitches
    {

        private static int _allowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactories;
        internal const string AllowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactoriesName = @"Switch.System.Configuration.AllowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactories";

        public static bool AllowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactories
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(AllowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactoriesName, ref _allowUserConfigFilesToLoadWhenSearchingForWellKnownSqlClientFactories);
            }
        }
    }
}

