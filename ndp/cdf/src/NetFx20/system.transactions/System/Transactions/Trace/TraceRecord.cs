//-----------------------------------------------------------------------------
// <copyright file="TraceRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions.Diagnostics
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// Base class for the team-specific traces that contain structured data.
    /// </summary>
    internal abstract class TraceRecord
    {
        internal protected const string EventIdBase = "http://schemas.microsoft.com/2004/03/Transactions/";
        internal protected const string NamespaceSuffix = "TraceRecord";
        
        /// <summary>
        /// Defines object layout.
        /// </summary>
        internal virtual string EventId { get { return EventIdBase + "Empty" + TraceRecord.NamespaceSuffix; } }
        
        public override string ToString()
        {
            PlainXmlWriter xml = new PlainXmlWriter();
            WriteTo(xml);
            return xml.ToString();
        }

        internal abstract void WriteTo(XmlWriter xml);
    }
}
