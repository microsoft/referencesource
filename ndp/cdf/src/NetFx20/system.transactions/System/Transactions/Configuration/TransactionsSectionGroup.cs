//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace System.Transactions.Configuration
{
    using System;
    using System.Configuration;

    public sealed class TransactionsSectionGroup : ConfigurationSectionGroup
    {
        public TransactionsSectionGroup()
        {
        }

        [ConfigurationProperty(ConfigurationStrings.DefaultSettingsSectionName)]
        public DefaultSettingsSection DefaultSettings
        {
            get { return (DefaultSettingsSection)this.Sections[ConfigurationStrings.DefaultSettingsSectionName]; }
        }

        static public TransactionsSectionGroup GetSectionGroup(Configuration config)
        {
            if (config == null)
            {
                throw new ArgumentNullException("config");
            }
            return (TransactionsSectionGroup)config.GetSectionGroup(ConfigurationStrings.SectionGroupName);
        }

        [ConfigurationProperty(ConfigurationStrings.MachineSettingsSectionName)]
        public MachineSettingsSection MachineSettings
        {
            get { return (MachineSettingsSection)this.Sections[ConfigurationStrings.MachineSettingsSectionName]; }
        }
    }
}
