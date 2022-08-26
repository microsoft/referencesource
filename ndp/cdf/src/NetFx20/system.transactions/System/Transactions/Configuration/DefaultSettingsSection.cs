//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace System.Transactions.Configuration
{
    using System;
    using System.Configuration;
    using System.Globalization;

    public sealed class DefaultSettingsSection : ConfigurationSection
    {
        public DefaultSettingsSection() : base()
        {
        }

        [ConfigurationProperty(ConfigurationStrings.DistributedTransactionManagerName, DefaultValue = ConfigurationStrings.DefaultDistributedTransactionManagerName)]
        public string DistributedTransactionManagerName
        {
            get { return (string)base[ConfigurationStrings.DistributedTransactionManagerName]; }
            set
            {
                base[ConfigurationStrings.DistributedTransactionManagerName] = value;
            }
        }

        static internal DefaultSettingsSection GetSection()
        {
            DefaultSettingsSection retval = (DefaultSettingsSection)PrivilegedConfigurationManager.GetSection(ConfigurationStrings.DefaultSettingsSectionPath);
            if (retval == null)
            {
                throw new ConfigurationErrorsException(string.Format(CultureInfo.CurrentCulture,
                    SR.GetString(SR.ConfigurationSectionNotFound),
                    ConfigurationStrings.DefaultSettingsSectionPath));
            }
            return retval;
        }

        [ConfigurationProperty(ConfigurationStrings.Timeout, DefaultValue = ConfigurationStrings.DefaultTimeout)]
        [TimeSpanValidator(MinValueString = ConfigurationStrings.TimeSpanZero, MaxValueString = TimeSpanValidatorAttribute.TimeSpanMaxValue)]
        public TimeSpan Timeout
        {
            get { return (TimeSpan)base[ConfigurationStrings.Timeout]; }
            set
            {
                if (!ConfigurationStrings.IsValidTimeSpan(value))
                {
                    throw new ArgumentOutOfRangeException("Timeout", SR.GetString(SR.ConfigInvalidTimeSpanValue));
                }

                base[ConfigurationStrings.Timeout] = value;
            }
        }

        protected override ConfigurationPropertyCollection Properties
        {
            get
            {
                ConfigurationPropertyCollection retval = new ConfigurationPropertyCollection();
                retval.Add(new ConfigurationProperty(ConfigurationStrings.DistributedTransactionManagerName, typeof(string), ConfigurationStrings.DefaultDistributedTransactionManagerName, ConfigurationPropertyOptions.None));
                retval.Add(new ConfigurationProperty(ConfigurationStrings.Timeout, 
                                                         typeof(TimeSpan), 
                                                         ConfigurationStrings.DefaultTimeout,
                                                         null,
                                                         new TimeSpanValidator( TimeSpan.Zero, TimeSpan.MaxValue ),
                                                         ConfigurationPropertyOptions.None));
                return retval;
            }
        }
    }
}
