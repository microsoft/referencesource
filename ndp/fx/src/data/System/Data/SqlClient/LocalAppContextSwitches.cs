// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Runtime.CompilerServices;

namespace System.Data.SqlClient
{
    internal static class LocalAppContextSwitches
    {
        internal const string MakeReadAsyncBlockingString = @"Switch.System.Data.SqlClient.MakeReadAsyncBlocking";
        private static int _makeReadAsyncBlocking;
        public static bool MakeReadAsyncBlocking
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(MakeReadAsyncBlockingString, ref _makeReadAsyncBlocking);
            }
        }

        internal const string UseMinimumLoginTimeoutString = @"Switch.System.Data.SqlClient.UseOneSecFloorInTimeoutCalculationDuringLogin";
        private static int _useMinimumLoginTimeout;
        public static bool UseMinimumLoginTimeout
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(UseMinimumLoginTimeoutString, ref _useMinimumLoginTimeout);
            }
        }

        internal const string DisableTNIRByDefaultString = @"Switch.System.Data.SqlClient.DisableTNIRByDefaultInConnectionString";
        private static int _disableTNIRByDefault;
        public static bool DisableTNIRByDefault
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DisableTNIRByDefaultString, ref _disableTNIRByDefault);
            }
        }
    }
}
