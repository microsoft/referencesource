// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.VisualStudio.Activities.ImportServiceContract
{
    using System.Globalization;

    internal static class ImportServiceContractNamingHelper
    {
        internal static string GenerateNameWithSuffix(string baseString, string format, int suffix)
        {
            if (suffix == 1)
            {
                return baseString;
            }
            else
            {
                return string.Format(CultureInfo.InvariantCulture, format, baseString, suffix);
            }
        }
    }
}