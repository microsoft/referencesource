//---------------------------------------------------------------------
// <copyright file="EpmSourcePathSegment.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Type describing each node in the EpmSourceTree generated using
// EntityPropertyMappingAttributes for a ResourceType.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System.Collections.Generic;

    /// <summary>
    /// Representation of each node in the <see cref="EpmSourceTree"/>
    /// </summary>
    internal class EpmSourcePathSegment
    {
        #region Fields

        /// <summary>Name of the property under the parent resource type</summary>
        private String propertyName;

        /// <summary>List of sub-properties if this segment corresponds to a complex type</summary>
        private List<EpmSourcePathSegment> subProperties;

        #endregion

        /// <summary>
        /// Constructor creates a source path segment with the name set to <paramref name="propertyName"/>
        /// </summary>
        /// <param name="propertyName">Segment property name</param>
        internal EpmSourcePathSegment(String propertyName)
        {
            this.propertyName = propertyName;
            this.subProperties = new List<EpmSourcePathSegment>();
        }

        #region Properties

        /// <summary>Name of the property under the parent resource type</summary>
        internal String PropertyName
        {
            get
            {
                return this.propertyName;
            }
        }

        /// <summary>List of sub-properties if this segment corresponds to a complex type</summary>
        internal List<EpmSourcePathSegment> SubProperties
        {
            get
            {
                return this.subProperties;
            }
        }

        /// <summary>Corresponding EntityPropertyMappingInfo</summary>
        internal EntityPropertyMappingInfo EpmInfo
        {
            get;
            set;
        }

        #endregion
    }
}
