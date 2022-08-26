﻿//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: AppContextDefaultValues.cs
//---------------------------------------------------------------------------

using System.Windows;
using MS.Internal;

namespace System
{
    // WPF's builds are seeing new warnings as as result of using LocalAppContext in PresentationFramework, PresentationCore and WindowsBase. 
    // These binaries have internalsVisibleTo attribute set between them - which results in the warning. 
    // We don't have a way of suppressing this warning effectively until the shared copies of LocalAppContext and 
    // AppContextDefaultValues have pragmas added to suppress warning 436
#pragma warning disable 436
    internal static partial class AppContextDefaultValues
    {
        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int targetFrameworkVersion)
        {
            switch (platformIdentifier)
            {
                case ".NETFramework":
                    {
                        if (targetFrameworkVersion <= 40601)
                        {
                            LocalAppContext.DefineSwitchDefault(CoreAppContextSwitches.DoNotScaleForDpiChangesSwitchName, true);
                        }

                        if (targetFrameworkVersion <= 40602)
                        {
                            LocalAppContext.DefineSwitchDefault(CoreAppContextSwitches.OverrideExceptionWithNullReferenceExceptionName, true);
                        }

                        if (targetFrameworkVersion <= 40702)
                        {
                            LocalAppContext.DefineSwitchDefault(CoreAppContextSwitches.DoNotUsePresentationDpiCapabilityTier2OrGreaterSwitchName, true);
                        }

                        break;
                    }
            }
        }
    }
#pragma warning restore 436
}
