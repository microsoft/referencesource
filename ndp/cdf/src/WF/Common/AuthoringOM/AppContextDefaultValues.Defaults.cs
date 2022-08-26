// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System
{
    internal static partial class AppContextDefaultValues
    {
        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int version)
        {
            switch (platformIdentifier)
            {
                case ".NETCore":
                case ".NETFramework":
                    {
                        // All previous versions of that platform (up thru 4.7.2) will get the old behavior by default 
                        if (version <= 40702)
                        {
                            // This switch needs to be set in the MSBuild.Exe.Config that is used to build the project.
                            LocalAppContext.DefineSwitchDefault(@"Switch.System.Workflow.ComponentModel.UseLegacyHashForXomlFileChecksum", true);
                        }
                        
                        break;
                    }
            }
        }
    }
}
