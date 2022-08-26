//---------------------------------------------------------------------
// <copyright file="MediaEntryAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Attribute to denote entity types describing a media entry
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    /// This class marks a type that represents an Astoria client entity
    /// such that the Astoria client will treat it as a media entry 
    /// according to ATOM's "media link entry" concept.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class MediaEntryAttribute : Attribute
    {
        /// <summary>Name of the member that contains the data for the media entry</summary>
        private readonly string mediaMemberName;

        /// <summary>
        /// Creates a new MediaEntryAttribute attribute and sets the name
        /// of the member that contains the actual data of the media entry
        /// (e.g. a byte[] containing a picture, a string containing HTML, etc.)
        /// </summary>
        /// <param name="mediaMemberName">Name of the member that contains the data for the media entry</param>
        public MediaEntryAttribute(string mediaMemberName)
        {
            Util.CheckArgumentNull(mediaMemberName, "mediaMemberName");
            this.mediaMemberName = mediaMemberName;
        }

        /// <summary>Name of the member that contains the data for the media entry</summary>
        public string MediaMemberName
        {
            get { return this.mediaMemberName; }
        }
    }
}
