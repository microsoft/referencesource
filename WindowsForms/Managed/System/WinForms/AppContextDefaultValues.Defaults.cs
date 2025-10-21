//------------------------------------------------------------------------------
// <copyright file="AppContextDefaultValues.Defaults.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System {
    using System.Windows.Forms;

    internal static partial class AppContextDefaultValues {
        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int version) {
            // When defining a new switch  you should add it to the last known version.
            // For instance, if you are adding a switch in .NET 4.6.1 (the release after 4.6) you should define your switch
            // like this:
            //    if (version <= 40600) ...
            // This ensures that all previous versions of that platform (up-to 4.6) will get the old behavior by default
            // NOTE: When adding a default value for a switch please make sure that the default value is added to ALL of the existing platforms!
            // NOTE: When adding a new if statement for the version please ensure that ALL previous switches are enabled (i.e. don't use else if)
            switch (platformIdentifier) {
                case ".NETFramework": {
                    if (version <= 40600) {
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.DontSupportReentrantFilterMessageSwitchName, true);
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.DoNotSupportSelectAllShortcutInMultilineTextBoxSwitchName, true);
                    }
                    if (version <= 40602) {
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.DoNotLoadLatestRichEditControlSwitchName, true);
                    }
                    if (version <= 40700) {
                        LocalAppContext.DefineSwitchDefault(AccessibilityImprovements.UseLegacyAccessibilityFeaturesSwitchName, true);
                    }
                    if (version <= 40701) {
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.UseLegacyContextMenuStripSourceControlValueSwitchName, true);
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.DomainUpDownUseLegacyScrollingSwitchName, true);
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.AllowUpdateChildControlIndexForTabControlsSwitchName, true);
                        LocalAppContext.DefineSwitchDefault(AccessibilityImprovements.UseLegacyAccessibilityFeatures2SwitchName, true);
                    }
                    if (version <= 40702) {
                        LocalAppContext.DefineSwitchDefault(AccessibilityImprovements.UseLegacyAccessibilityFeatures3SwitchName, true);
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.UseLegacyImagesSwitchName, true);
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.EnableVisualStyleValidationSwitchName, true);
                    }

                    // When this switch is set to false, it enables the keyboard tooltips feature which is an "opt-in" feature.
                    // UI that depends on this feature should be designed appropriately to prevent clutter.
                    // That's why this switch is set to true by default for all versions of the framework.
                    LocalAppContext.DefineSwitchDefault(AccessibilityImprovements.UseLegacyToolTipDisplaySwitchName, true);

                    break;
                }
            }
        }
    }
}
