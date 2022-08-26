//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using System.Collections.ObjectModel;

    public sealed class ServiceHealthSectionCollection : Collection<ServiceHealthSection>
    {
        public ServiceHealthSection CreateSection(string title)
        {
            if (title == null)
            {
                throw new ArgumentNullException(nameof(title));
            }

            ServiceHealthSection section = new ServiceHealthSection(title);
            this.Add(section);
            return section;
        }

        public ServiceHealthSection CreateSection(string title, string backgroundColor)
        {
            if (backgroundColor == null)
            {
                throw new ArgumentNullException(nameof(backgroundColor));
            }

            ServiceHealthSection section = CreateSection(title);
            section.BackgroundColor = backgroundColor;
            return section;
        }

        public ServiceHealthSection CreateSection(string title, string backgroundColor, string foregroundColor)
        {
            if (foregroundColor == null)
            {
                throw new ArgumentNullException(nameof(foregroundColor));
            }

            ServiceHealthSection section = CreateSection(title, backgroundColor);
            section.ForegroundColor = foregroundColor;
            return section;
        }
    }
}

