//------------------------------------------------------------------------------
// <copyright file="SystemDrawingSection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Configuration {
    using System;
    using System.Configuration;

    /// <include file='doc\SystemDrawingSection.uex' path='docs/doc[@for="SystemDrawingSection"]/*' />
    /// <devdoc>
    /// A configuration section with a "bitmapSuffix" string value that specifies the suffix to be
    /// appended to bitmaps that are loaded through ToolboxBitmapAttribute and similar attributes.
    /// </devdoc>
    public sealed class SystemDrawingSection : ConfigurationSection {

        private const string BitmapSuffixSectionName = "bitmapSuffix";

        static SystemDrawingSection() {
            properties.Add(bitmapSuffix);
        }

        [ConfigurationProperty(BitmapSuffixSectionName)]
        public string BitmapSuffix {
            get { return (string)this[bitmapSuffix]; }
            set { this[bitmapSuffix] = value; }
        }

        protected override ConfigurationPropertyCollection Properties {
            get { return properties; }
        }

        private static readonly ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

        private static readonly ConfigurationProperty bitmapSuffix =
            new ConfigurationProperty(BitmapSuffixSectionName, typeof(string), null, ConfigurationPropertyOptions.None);
    }
}
