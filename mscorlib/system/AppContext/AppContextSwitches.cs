// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace System
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class AppContextSwitches
    {
        private static int _noAsyncCurrentCulture;
        public static bool NoAsyncCurrentCulture
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchNoAsyncCurrentCulture, ref _noAsyncCurrentCulture);
            }
        }

        private static int _enforceJapaneseEraYearRanges;
        public static bool EnforceJapaneseEraYearRanges
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchEnforceJapaneseEraYearRanges, ref _enforceJapaneseEraYearRanges);
            }
        }

        private static int _formatJapaneseFirstYearAsANumber;
        public static bool FormatJapaneseFirstYearAsANumber
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchFormatJapaneseFirstYearAsANumber, ref _formatJapaneseFirstYearAsANumber);
            }
        }

        private static int _enforceLegacyJapaneseDateParsing;
        public static bool EnforceLegacyJapaneseDateParsing
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchEnforceLegacyJapaneseDateParsing, ref _enforceLegacyJapaneseDateParsing);
            }
        }

        private static int _throwExceptionIfDisposedCancellationTokenSource;
        public static bool ThrowExceptionIfDisposedCancellationTokenSource
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchThrowExceptionIfDisposedCancellationTokenSource, ref _throwExceptionIfDisposedCancellationTokenSource);
            }
        }

        private static int _useConcurrentFormatterTypeCache;
        public static bool UseConcurrentFormatterTypeCache
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchUseConcurrentFormatterTypeCache, ref _useConcurrentFormatterTypeCache);
            }
        }

        private static int _preserveEventListnerObjectIdentity;
        public static bool PreserveEventListnerObjectIdentity
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchPreserveEventListnerObjectIdentity, ref _preserveEventListnerObjectIdentity);
            }
        }

        private static int _useLegacyPathHandling;

        /// <summary>
        /// Use legacy path normalization logic and blocking of extended syntax.
        /// </summary>
        public static bool UseLegacyPathHandling
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchUseLegacyPathHandling, ref _useLegacyPathHandling);
            }
        }

        private static int _blockLongPaths;

        /// <summary>
        /// Throw PathTooLongException for paths greater than MAX_PATH or directories greater than 248 (as per CreateDirectory Win32 limitations)
        /// </summary>
        public static bool BlockLongPaths
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchBlockLongPaths, ref _blockLongPaths);
            }
        }

        private static int _cloneActor;

        /// <summary>
        /// When copying a ClaimsIdentity.Actor this switch controls whether ClaimsIdentity.Actor should be set as a reference or the result of Actor.Clone()
        /// </summary>
        public static bool SetActorAsReferenceWhenCopyingClaimsIdentity
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchSetActorAsReferenceWhenCopyingClaimsIdentity, ref _cloneActor);
            }
        }

        private static int _doNotAddrOfCspParentWindowHandle;
        public static bool DoNotAddrOfCspParentWindowHandle
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchDoNotAddrOfCspParentWindowHandle, ref _doNotAddrOfCspParentWindowHandle);
            }
        }

        private static int _ignorePortablePDBsInStackTraces;
        public static bool IgnorePortablePDBsInStackTraces
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchIgnorePortablePDBsInStackTraces, ref _ignorePortablePDBsInStackTraces);
            }
        }

        private static int _useNewMaxArraySize;
        public static bool UseNewMaxArraySize
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchUseNewMaxArraySize, ref _useNewMaxArraySize);
            }
        }

        private static int _useLegacyExecutionContextBehaviorUponUndoFailure;
        public static bool UseLegacyExecutionContextBehaviorUponUndoFailure
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchUseLegacyExecutionContextBehaviorUponUndoFailure, ref _useLegacyExecutionContextBehaviorUponUndoFailure);
            }
        }

        private static int _useLegacyFipsThrow;
        public static bool UseLegacyFipsThrow
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchCryptographyUseLegacyFipsThrow, ref _useLegacyFipsThrow);
            }
        }

        private static int _doNotMarshalOutByrefSafeArrayOnInvoke;
        public static bool DoNotMarshalOutByrefSafeArrayOnInvoke
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchDoNotMarshalOutByrefSafeArrayOnInvoke, ref _doNotMarshalOutByrefSafeArrayOnInvoke);
            }
        }

        private static int _useNetCoreTimer;
        public static bool UseNetCoreTimer
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return GetCachedSwitchValue(AppContextDefaultValues.SwitchUseNetCoreTimer, ref _useNetCoreTimer);
            }
        }

        //
        // Implementation details
        //

        private static bool DisableCaching { get; set; }

        static AppContextSwitches()
        {
            bool isEnabled;
            if (AppContext.TryGetSwitch(@"TestSwitch.LocalAppContext.DisableCaching", out isEnabled))
            {
                DisableCaching = isEnabled;
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static bool GetCachedSwitchValue(string switchName, ref int switchValue)
        {
            if (switchValue < 0) return false;
            if (switchValue > 0) return true;

            return GetCachedSwitchValueInternal(switchName, ref switchValue);
        }

        private static bool GetCachedSwitchValueInternal(string switchName, ref int switchValue)
        {
            bool isSwitchEnabled;
            AppContext.TryGetSwitch(switchName, out isSwitchEnabled);

            if (DisableCaching)
            {
                return isSwitchEnabled;
            }

            switchValue = isSwitchEnabled ? 1 /*true*/ : -1 /*false*/;
            return isSwitchEnabled;
        }
    }
}
