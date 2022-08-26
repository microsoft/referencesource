// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
#pragma warning disable 0436 //Disable the type conflict warning for the types used by LocalAppContext framework due to InternalsVisibleTo for System.Workflow.ComponentModel (Quirking)
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
                            LocalAppContext.DefineSwitchDefault(@"Switch.System.Workflow.Runtime.UseLegacyHashForWorkflowDefinitionDispenserCacheKey", true);
                            LocalAppContext.DefineSwitchDefault(@"Switch.System.Workflow.Runtime.UseLegacyHashForSqlTrackingCacheKey", true);
                        }

                        break;
                    }
            }
        }
    }
}
