// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System.Messaging
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int useMD5ForDefaultHashAlgorithm;

        public static bool UseMD5ForDefaultHashAlgorithm
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(System.AppContextDefaultValues.UseMD5ForDefaultHashAlgorithmSwitchString, ref useMD5ForDefaultHashAlgorithm);
            }
        }
    }
}
