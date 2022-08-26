//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    public sealed class ServiceHealthData
    {
        private string key;

        public ServiceHealthData(string key, string[] values)
        {
            if (string.IsNullOrWhiteSpace(key))
            {
                throw new ArgumentNullException(nameof(key));
            }

            this.Key = key;
            this.Values = values;
        }

        public string Key
        {
            get
            {
                return this.key;
            }

            set
            {
                if (string.IsNullOrWhiteSpace(value))
                {
                    throw new ArgumentNullException(nameof(Key));
                }

                this.key = value;
            }
        }

        public string[] Values { get; set; }
    }
}

