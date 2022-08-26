// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System
{
    using System;

    internal static partial class AppContextDefaultValues
    {
        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int version)
        {
            switch (platformIdentifier)
            {
                case ".NETCore":
                case ".NETFramework":
                    {
                        // All previous versions of that platform (up-to 4.6.2) will get the old behavior by default 
                        if (version <= 40602)
                        {
                            LocalAppContext.DefineSwitchDefault("Switch.System.ServiceModel.Internals.IncludeNullExceptionMessageInETWTrace", true);
                        }
                        
                        break;
                    }
            }
        }
    }
}
