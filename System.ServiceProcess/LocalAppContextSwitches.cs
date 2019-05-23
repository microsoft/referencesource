//------------------------------------------------------------------------------
// <copyright file="LocalAppContextSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System
{
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        internal const string DontThrowExceptionsOnStartName = @"Switch.System.ServiceProcess.DontThrowExceptionsOnStart";
        private static int _dontThrowExceptionsOnStart;

        public static bool DontThrowExceptionsOnStart
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontThrowExceptionsOnStartName, ref _dontThrowExceptionsOnStart);
            }
        }
    }
}       
