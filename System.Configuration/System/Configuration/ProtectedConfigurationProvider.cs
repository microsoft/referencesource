//------------------------------------------------------------------------------
// <copyright file="ProtectedConfigurationProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Collections.Specialized;
    using System.Runtime.Serialization;
    using System.Configuration.Provider;
    using System.Xml;
    using System.IO;

    public abstract class ProtectedConfigurationProvider : ProviderBase
    {
        public abstract XmlNode Encrypt(XmlNode node);
        public abstract XmlNode Decrypt(XmlNode encryptedNode);

        internal static void LoadXml(XmlDocument xmlDoc, string xmlText)
        {
            using (XmlTextReader reader = new XmlTextReader(new StringReader(xmlText)))
            {
                reader.DtdProcessing = DtdProcessing.Ignore;
                xmlDoc.Load(reader);
            }
        }
    }
}
