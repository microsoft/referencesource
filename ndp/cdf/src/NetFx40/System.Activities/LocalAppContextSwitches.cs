// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
#pragma warning disable 0436 //Disable the type conflict warning for the types used by LocalAppContext framework due to InternalsVisibleTo for System.ServiceModel.Internals (Quirking)
namespace System.Activities
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int useMD5ForWFDebugger;
        private static int useSHA1HashForDebuggerSymbols;
        
        public static bool UseMD5ForWFDebugger
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Activities.UseMD5ForWFDebugger", ref useMD5ForWFDebugger);
            }
        }

        public static bool UseSHA1HashForDebuggerSymbols
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.Activities.UseSHA1HashForDebuggerSymbols", ref useSHA1HashForDebuggerSymbols);
            }
        }
    }
}
