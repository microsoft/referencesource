//------------------------------------------------------------------------------
// <copyright file="TransactionsSection.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Transactions.Configuration
{
    using System.Configuration;
    using System.Globalization;

    internal static class ConfigurationStrings
    {
        internal const string DefaultDistributedTransactionManagerName = "";
        internal const string DefaultMaxTimeout = "00:10:00";
        internal const string DefaultTimeout = "00:01:00";
        internal const string TimeSpanZero = "00:00:00";

        static internal string DefaultSettingsSectionPath
        {
            get { return ConfigurationStrings.GetSectionPath(ConfigurationStrings.DefaultSettingsSectionName); }
        }

        static internal string GetSectionPath(string sectionName)
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}/{1}",
                ConfigurationStrings.SectionGroupName,
                sectionName);
        }

        static internal bool IsValidTimeSpan(TimeSpan span)
        {
            return (span >= TimeSpan.Zero);
        }

        static internal string MachineSettingsSectionPath
        {
            get { return ConfigurationStrings.GetSectionPath(ConfigurationStrings.MachineSettingsSectionName); }
        }

        internal const string DefaultSettingsSectionName = "defaultSettings";
        internal const string DistributedTransactionManagerName = "distributedTransactionManagerName";
        internal const string MaxTimeout = "maxTimeout";
        internal const string MachineSettingsSectionName = "machineSettings";
        internal const string SectionGroupName = "system.transactions";
        internal const string Timeout = "timeout";
    }
}
