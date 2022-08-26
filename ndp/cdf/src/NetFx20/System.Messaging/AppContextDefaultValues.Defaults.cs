// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
namespace System
{
    internal static partial class AppContextDefaultValues
    {
        internal const string UseMD5ForDefaultHashAlgorithmSwitchString = @"Switch.System.Messaging.UseMD5ForDefaultHashAlgorithm";

        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int version)
        {
            switch (platformIdentifier)
            {
                case ".NETFramework":
                    {
                        // All previous versions of that platform (up-to 4.7) will get the old behavior by default 
                        if (version <= 40700)
                        {
                            LocalAppContext.DefineSwitchDefault(UseMD5ForDefaultHashAlgorithmSwitchString, true);
                        }

                        break;
                    }
            }
        }
    }
}
