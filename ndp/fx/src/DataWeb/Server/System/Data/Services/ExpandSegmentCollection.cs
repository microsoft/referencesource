//---------------------------------------------------------------------
// <copyright file="ExpandSegmentCollection.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a description of a path in an $expand query option
//      for a WCF Data Service.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Collections.Generic;

    /// <summary>
    /// Provides a description of a path in an $expand query option
    /// for a WCF Data Service.
    /// </summary>
    public class ExpandSegmentCollection : List<ExpandSegment>
    {
        /// <summary>Initializes a new <see cref="ExpandSegmentCollection"/> instance.</summary>
        public ExpandSegmentCollection()
        {
        }

        /// <summary>Initializes a new <see cref="ExpandSegmentCollection"/> instance.</summary>
        /// <param name='capacity'>Initial capacity.</param>
        public ExpandSegmentCollection(int capacity) : base(capacity)
        {
        }

        /// <summary>Whether this path has any filters.</summary>
        public bool HasFilter
        {
            get
            {
                foreach (ExpandSegment segment in this)
                {
                    if (segment.HasFilter)
                    {
                        return true;
                    }
                }

                return false;
            }
        }
    }
}
