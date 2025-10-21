//------------------------------------------------------------------------------
// <copyright file="LocalAppContextSwitches.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches {
        internal const string DontSupportReentrantFilterMessageSwitchName = @"Switch.System.Windows.Forms.DontSupportReentrantFilterMessage";
        internal const string DoNotSupportSelectAllShortcutInMultilineTextBoxSwitchName = @"Switch.System.Windows.Forms.DoNotSupportSelectAllShortcutInMultilineTextBox";
        internal const string DoNotLoadLatestRichEditControlSwitchName = @"Switch.System.Windows.Forms.DoNotLoadLatestRichEditControl";
        internal const string UseLegacyContextMenuStripSourceControlValueSwitchName = @"Switch.System.Windows.Forms.UseLegacyContextMenuStripSourceControlValue";
        internal const string DomainUpDownUseLegacyScrollingSwitchName = @"Switch.System.Windows.Forms.DomainUpDown.UseLegacyScrolling";
        internal const string AllowUpdateChildControlIndexForTabControlsSwitchName = @"Switch.System.Windows.Forms.AllowUpdateChildControlIndexForTabControls";
        internal const string UseLegacyImagesSwitchName = @"Switch.System.Windows.Forms.UseLegacyImages";
        internal const string EnableVisualStyleValidationSwitchName = @"Switch.System.Windows.Forms.EnableVisualStyleValidation";
        internal const string EnableLegacyDangerousClipboardDeserializationModeSwitchName = @"Switch.System.Windows.Forms.EnableLegacyDangerousClipboardDeserializationMode";
        internal const string EnableLegacyChineseIMEIndicatorSwitchName = @"Switch.System.Windows.Forms.EnableLegacyChineseIMEIndicator";
        internal const string EnableLegacyIMEFocusInComboBoxSwitchName = @"Switch.System.Windows.Forms.EnableLegacyIMEFocusInComboBox";

        private static int _dontSupportReentrantFilterMessage;
        private static int _doNotSupportSelectAllShortcutInMultilineTextBox;
        private static int _doNotLoadLatestRichEditControl;
        private static int _useLegacyContextMenuStripSourceControlValue;
        private static int _useLegacyDomainUpDownScrolling;
        private static int _allowUpdateChildControlIndexForTabControls;
        private static int _useLegacyImages;
        private static int _enableVisualStyleValidation;
        private static int _enableLegacyDangerousClipboardDeserializationMode;
        private static int _enableLegacyChineseIMEIndicator;
        private static int _enableLegacyIMEFocusInComboBox;

        public static bool DontSupportReentrantFilterMessage {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.DontSupportReentrantFilterMessageSwitchName, ref _dontSupportReentrantFilterMessage);
            }
        }

        public static bool DoNotSupportSelectAllShortcutInMultilineTextBox {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.DoNotSupportSelectAllShortcutInMultilineTextBoxSwitchName, ref _doNotSupportSelectAllShortcutInMultilineTextBox);
            }
        }

        public static bool DoNotLoadLatestRichEditControl {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.DoNotLoadLatestRichEditControlSwitchName, ref _doNotLoadLatestRichEditControl);
            }
        }

        public static bool UseLegacyContextMenuStripSourceControlValue {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.UseLegacyContextMenuStripSourceControlValueSwitchName, ref _useLegacyContextMenuStripSourceControlValue);
            }
        }

        public static bool UseLegacyDomainUpDownControlScrolling {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.DomainUpDownUseLegacyScrollingSwitchName, ref _useLegacyDomainUpDownScrolling);
            }
        }

        public static bool AllowUpdateChildControlIndexForTabControls {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.AllowUpdateChildControlIndexForTabControlsSwitchName, ref _allowUpdateChildControlIndexForTabControls);
            }
        }

        public static bool UseLegacyImages {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.UseLegacyImagesSwitchName, ref _useLegacyImages);
            }
        }

        public static bool EnableVisualStyleValidation {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.EnableVisualStyleValidationSwitchName, ref _enableVisualStyleValidation);
            }
        }

        /// <summary>
        /// This AppContext switch controls reading from the clipboard. When reading a "System.String", "System.Drawing.Bitmap",
        /// or an OLE format, we restrict deserialization to primitive types, thus blocking malicious code execution.
        /// </summary>
        public static bool EnableLegacyDangerousClipboardDeserializationMode {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                if (_enableLegacyDangerousClipboardDeserializationMode < 0) return false;
                if (_enableLegacyDangerousClipboardDeserializationMode > 0) return true;

                // Device guard overrides the app context value when enabled. 
                if (UnsafeNativeMethods.IsDynamicCodePolicyEnabled()) {
                    _enableLegacyDangerousClipboardDeserializationMode = -1;
                } else {
                    LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.EnableLegacyDangerousClipboardDeserializationModeSwitchName, ref _enableLegacyDangerousClipboardDeserializationMode);
                }

                return (_enableLegacyDangerousClipboardDeserializationMode > 0);
            }
        }

        /// <summary>
        /// When activated, this quirk enables controls with ImeMode.NoControl in Chinese IME to accept input according to the current OS setting. 
        /// This fix is on by default. Please set this value to true in app.config file in order to opt-out and get the 4.5.1 behavior.
        /// </summary>
        public static bool EnableLegacyChineseIMEIndicator {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.EnableLegacyChineseIMEIndicatorSwitchName, ref _enableLegacyChineseIMEIndicator);
            }
        }

        /// <summary>
        /// When activated, this quirk enables ComboBoxes with ImeMode.NoControl in Chinese IME Pinyin to accept input according to the current OS setting, 
        /// when setting focus to this control with a mouse click. This fix is on by default. Please set this value to true in app.config file. 
        /// </summary>
        public static bool EnableLegacyIMEFocusInComboBox {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get {
                return LocalAppContext.GetCachedSwitchValue(LocalAppContextSwitches.EnableLegacyIMEFocusInComboBoxSwitchName, ref _enableLegacyIMEFocusInComboBox);
            }
        }

    }
}
