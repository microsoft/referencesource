// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System.ServiceModel.Internals
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int includeNullExceptionMessageInETWTrace;
        
        public static bool IncludeNullExceptionMessageInETWTrace
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(@"Switch.System.ServiceModel.Internals.IncludeNullExceptionMessageInETWTrace", ref includeNullExceptionMessageInETWTrace);
            }
        }
    }
}
