//---------------------------------------------------------------------
// <copyright file="EpmTargetPathSegment.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Type describing each node in the EpmTargetTree generated using
// EntityPropertyMappingAttributes for a ResourceType.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System.Diagnostics;
    using System.Collections.Generic;

    /// <summary>
    /// Representation of each node in the <see cref="EpmTargetTree"/>
    /// </summary>
    [DebuggerDisplay("EpmTargetPathSegment {SegmentName} HasContent={HasContent}")]
    internal class EpmTargetPathSegment
    {
        #region Private fields.

        /// <summary>Name of the xml element/attribute</summary>
        private String segmentName;

        /// <summary>URI of the namespace to which the <see cref="segmentName"/> belongs</summary>
        private String segmentNamespaceUri;

        /// <summary>Prefix to be used in xml document for <see cref="segmentNamespaceUri"/></summary>
        private String segmentNamespacePrefix;

        /// <summary>If this is a non-leaf element, the child elements/attributes collection</summary>
        private List<EpmTargetPathSegment> subSegments;

        /// <summary>Parent element of this element/attribute</summary>
        private EpmTargetPathSegment parentSegment;

        #endregion Private fields.

        /// <summary>
        /// Constructor initializes the list of sub-nodes to be empty, used for creating root nodes
        /// in the <see cref="EpmTargetTree"/>
        /// </summary>
        internal EpmTargetPathSegment()
        {
            this.subSegments = new List<EpmTargetPathSegment>();
        }

        /// <summary>Used for creating non-root nodes in the syndication/custom trees</summary>
        /// <param name="segmentName">Name of xml element/attribute</param>
        /// <param name="segmentNamespaceUri">URI of the namespace for <paramref name="segmentName"/></param>
        /// <param name="segmentNamespacePrefix">Namespace prefix to be used for <paramref name="segmentNamespaceUri"/></param>
        /// <param name="parentSegment">Reference to the parent node if this is a sub-node, useful for traversals in visitors</param>
        internal EpmTargetPathSegment(String segmentName, String segmentNamespaceUri, String segmentNamespacePrefix, EpmTargetPathSegment parentSegment)
            : this()
        {
            this.segmentName = segmentName;
            this.segmentNamespaceUri = segmentNamespaceUri;
            this.segmentNamespacePrefix = segmentNamespacePrefix;
            this.parentSegment = parentSegment;
        }

        /// <summary>Name of the xml element/attribute</summary>
        internal String SegmentName
        {
            get
            {
                return this.segmentName;
            }
        }

        /// <summary>URI of the namespace to which the <see cref="segmentName"/> belongs</summary>
        internal String SegmentNamespaceUri
        {
            get
            {
                return this.segmentNamespaceUri;
            }
        }

        /// <summary>Prefix to be used in xml document for <see cref="segmentNamespaceUri"/></summary>
        internal String SegmentNamespacePrefix
        {
            get
            {
                return this.segmentNamespacePrefix;
            }
        }

        /// <summary>EntityPropertyMappingInfo corresponding to current segement</summary>
        internal EntityPropertyMappingInfo EpmInfo
        {
            get;
            set;
        }

        /// <summary>Whether this node corresponds to ResourceType or ClientType property values</summary>
        internal bool HasContent
        {
            get
            {
                return this.EpmInfo != null;
            }
        }

        /// <summary>Does this node correspond to xml attribute</summary>
        internal bool IsAttribute
        {
            get
            {
                return this.SegmentName[0] == '@';
            }
        }

        /// <summary>Parent node in the tree (always an element if present)</summary>
        internal EpmTargetPathSegment ParentSegment
        {
            get
            {
                return this.parentSegment;
            }
        }

        /// <summary>Sub-nodes of this node. Only exist if current node is an element node</summary>
        internal List<EpmTargetPathSegment> SubSegments
        {
            get
            {
                return this.subSegments;
            }
        }
    }
}
