//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using System.Collections.ObjectModel;

    public sealed class ServiceHealthDataCollection : KeyedCollection<string, ServiceHealthData>
    {
        public ServiceHealthDataCollection()
            : base(StringComparer.InvariantCultureIgnoreCase)
        {
        }

        protected override string GetKeyForItem(ServiceHealthData element)
        {
            return element.Key;
        }

        public void Add(string key, string value)
        {
            if (key == null)
            {
                throw new ArgumentNullException(nameof(key));
            }

            this.InternalAdd(key, new string[] { value }, false);
        }

        public void Add(string key, string[] values)
        {
            if (key == null)
            {
                throw new ArgumentNullException(nameof(key));
            }

            this.InternalAdd(key, values, true);
        }

        private void InternalAdd(string key, string[] values, bool isArray)
        {
            if (isArray)
            {
                int length = values == null ? 1 : values.Length + 1;
                string[] tmp = new string[length];

                if (values != null && values.Length > 0)
                {
                    values.CopyTo(tmp, 0);
                }

                tmp[length - 1] = string.Empty;

                values = tmp;
            }

            this.Add(new ServiceHealthData(key, values));
        }
    }
}

