//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace System.Transactions.Configuration
{
    using System.Configuration;
    using System.Collections.Generic;
    using System.Globalization;

    public sealed class MachineSettingsSection : ConfigurationSection
    {
        public MachineSettingsSection() : base()
        {
        }

        // System.Transactions reads the Configuration information in a non destructive way.
        static internal MachineSettingsSection GetSection()
        {
            MachineSettingsSection retval = (MachineSettingsSection)PrivilegedConfigurationManager.GetSection(ConfigurationStrings.MachineSettingsSectionPath);
            if (retval == null)
            {
                throw new ConfigurationErrorsException(string.Format(CultureInfo.CurrentCulture,
                    SR.GetString(SR.ConfigurationSectionNotFound),
                    ConfigurationStrings.MachineSettingsSectionPath));
            }
            return retval;
        }

        // public properties
        [ConfigurationProperty(ConfigurationStrings.MaxTimeout, DefaultValue = ConfigurationStrings.DefaultMaxTimeout)]
        [TimeSpanValidator(MinValueString = ConfigurationStrings.TimeSpanZero, MaxValueString = TimeSpanValidatorAttribute.TimeSpanMaxValue)]
        public TimeSpan MaxTimeout
        {
            // System.Transactions reads the Configuration information in a non destructive way.
            get { return (TimeSpan)base[ConfigurationStrings.MaxTimeout]; }
            set
            {
                if (!ConfigurationStrings.IsValidTimeSpan(value))
                {
                    throw new ArgumentOutOfRangeException("MaxTimeout", SR.GetString(SR.ConfigInvalidTimeSpanValue));
                }

                base[ConfigurationStrings.MaxTimeout] = value;
            }
        }

        protected override ConfigurationPropertyCollection Properties
        {
            get 
            {
                ConfigurationPropertyCollection retval = new ConfigurationPropertyCollection();
                retval.Add(new ConfigurationProperty( ConfigurationStrings.MaxTimeout, 
                                                      typeof(TimeSpan), 
                                                      ConfigurationStrings.DefaultMaxTimeout, 
                                                      null,
                                                      new TimeSpanValidator( TimeSpan.Zero, TimeSpan.MaxValue ),
                                                      ConfigurationPropertyOptions.None));
                return retval;
            }
        }
    }
}
