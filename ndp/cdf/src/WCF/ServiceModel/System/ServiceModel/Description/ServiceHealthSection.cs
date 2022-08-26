//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using System.Collections.ObjectModel;

    public sealed class ServiceHealthSection : Collection<ServiceHealthDataCollection>
    {
        private static string DefaultForegroundColor = "#000000";
        private static string DefaultBackgroundColor = "#ffffff";

        private string backgroundColor;
        private string foregroundColor;
        private string title;

        public ServiceHealthSection()
        {
            this.BackgroundColor = DefaultBackgroundColor;
            this.ForegroundColor = DefaultForegroundColor;
        }

        public ServiceHealthSection(string title)
            : this()
        {
            this.Title = title;
        }

        public string BackgroundColor
        {
            get
            {
                return this.backgroundColor;
            }

            set
            {
                if (string.IsNullOrWhiteSpace(value))
                {
                    throw new ArgumentNullException(nameof(BackgroundColor));
                }

                this.backgroundColor = value;
            }
        }

        public string ForegroundColor
        {
            get
            {
                return this.foregroundColor;
            }

            set
            {
                if (string.IsNullOrWhiteSpace(value))
                {
                    throw new ArgumentNullException(nameof(ForegroundColor));
                }

                this.foregroundColor = value;
            }
        }

        public string Title
        {
            get
            {
                return this.title;
            }

            set
            {
                if (string.IsNullOrWhiteSpace(value))
                {
                    throw new ArgumentNullException(nameof(Title));
                }

                this.title = value;
            }
        }

        public ServiceHealthDataCollection CreateElementsCollection()
        {
            ServiceHealthDataCollection elementsCollection = new ServiceHealthDataCollection();
            this.Add(elementsCollection);
            return elementsCollection;
        }
    }
}

