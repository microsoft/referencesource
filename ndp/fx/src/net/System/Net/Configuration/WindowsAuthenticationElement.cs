//------------------------------------------------------------------------------
// <copyright file="WindowsAuthenticationElement.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net.Configuration
{
    using System;
    using System.Configuration;

    public sealed class WindowsAuthenticationElement : ConfigurationElement
    {
        private ConfigurationPropertyCollection properties;
        private readonly ConfigurationProperty defaultCredentialsHandleCacheSize;

        public WindowsAuthenticationElement()
        {
            defaultCredentialsHandleCacheSize =
                new ConfigurationProperty(
                    ConfigurationStrings.DefaultCredentialsHandleCacheSize, 
                    typeof(int), 
                    0,
                    null,
                    new CacheSizeValidator(),
                    ConfigurationPropertyOptions.None);

            properties = new ConfigurationPropertyCollection();
            properties.Add(defaultCredentialsHandleCacheSize);
        }

        protected override ConfigurationPropertyCollection Properties 
        {
            get 
            {
                return properties;
            }
        }

        [ConfigurationProperty(ConfigurationStrings.DefaultCredentialsHandleCacheSize, DefaultValue = 0)]
        public int DefaultCredentialsHandleCacheSize
        {
            get { return (int)this[defaultCredentialsHandleCacheSize]; }
            set { this[defaultCredentialsHandleCacheSize] = value; }
        }

        private class CacheSizeValidator : ConfigurationValidatorBase
        {
            public override bool CanValidate(Type type)
            {
                return type == typeof(int);
            }

            public override void Validate(object value)
            {
                int size = (int)value;

                if (size < 0)
                {
                    throw new ArgumentOutOfRangeException("value", size, 
                        SR.GetString(SR.ArgumentOutOfRange_Bounds_Lower_Upper, 0, Int32.MaxValue));
                }
            }
        }                                      
    }
}
