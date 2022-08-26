//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: AppContextDefaultValues.cs
//---------------------------------------------------------------------------

using System.Windows;
using MS.Internal;

namespace System
{
    // WPF's builds are seeing warnings as a result of using LocalAppContext in mutliple assemblies.
    // that have internalsVisibleTo attribute set between them - which results in the warning.
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
                        if (targetFrameworkVersion <= 40701)
                        {
                            LocalAppContext.DefineSwitchDefault(BuildTasksAppContextSwitches.DoNotUseSha256ForMarkupCompilerChecksumAlgorithmSwitchName, true);
                        }

                        break;
                    }
            }
        }
    }
#pragma warning restore 436
}
