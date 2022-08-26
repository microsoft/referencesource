// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
#pragma warning disable 0436 //Disable the type conflict warning for the types used by LocalAppContext framework (Quirking)
namespace System.Runtime.Serialization
{
    using System;
    using System.Runtime.CompilerServices;

    // When adding a quirk, name it such that false is new behavior and true is old behavior.
    // You are opting IN to old behavior. The new behavior is default.
    // For example, we want to enable the functionality to explicitly add a connection close header
    internal static class LocalAppContextSwitches
    {
        private const string DoNotUseTimeZoneInfoString = "Switch.System.Runtime.Serialization.DoNotUseTimeZoneInfo";

        private static int doNotUseTimeZoneInfoString;

        public static bool DoNotUseTimeZoneInfo
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DoNotUseTimeZoneInfoString, ref doNotUseTimeZoneInfoString);
            }
        }
        
        public static readonly string DoNotUseEcmaScriptV6EscapeControlCharacterKeyString = "Switch.System.Runtime.Serialization.DoNotUseECMAScriptV6EscapeControlCharacter";

        private static int doNotUseEcmaScriptV6EscapeControlCharacter;

        public static bool DoNotUseEcmaScriptV6EscapeControlCharacter
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DoNotUseEcmaScriptV6EscapeControlCharacterKeyString, ref doNotUseEcmaScriptV6EscapeControlCharacter);
            }
        }
    }

}
