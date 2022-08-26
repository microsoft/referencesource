//---------------------------------------------------------------------
// <copyright file="AtomFeed.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class to represent an ATOM feed as parsed and as it
// goes through the materialization pipeline.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Linq;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to represent an ATOM feed as it goes through
    /// the WCF Data Services materialization pipeline.
    /// </summary>
    internal class AtomFeed
    {
        /// <summary>
        /// Inline count found on the feed; null if not found yet.
        /// </summary>
        public long? Count 
        { 
            get; 
            set; 
        }

        /// <summary>
        /// The [atom:link rel="next"] element
        /// </summary>
        /// <remarks>
        /// If the value is null, then we have no seen a next link element for the feed yet.
        /// </remarks>
        public Uri NextLink
        {
            get;
            set;
        }

        /// <summary>Enumeration of entries on this feed; possibly null.</summary>
        /// <remarks>
        /// This property will typically be null for a top-level feed
        /// to allow streaming the contents (at some level at least);
        /// the property will be assigned for nested feeds.
        /// </remarks>
        public IEnumerable<AtomEntry> Entries 
        { 
            get; 
            set; 
        }
    }
}
