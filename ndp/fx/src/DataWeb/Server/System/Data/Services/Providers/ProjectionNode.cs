//---------------------------------------------------------------------
// <copyright file="ProjectionNode.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Represents a single node in the tree of projections
//      for queries with $expand and/or $select.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System;
    using System.Diagnostics;
    #endregion

    /// <summary>Class describing a single node on the tree of projections
    /// and expansions. This is the base class used for any projected property.</summary>
    [DebuggerDisplay("ProjectionNode {PropertyName}")]
    internal class ProjectionNode
    {
        #region Private fields
        /// <summary>The name of the property to project.</summary>
        /// <remarks>If this node represents the root of the projection tree, this name is an empty string.</remarks>
        private readonly string propertyName;

        /// <summary>The <see cref="ResourceProperty"/> for the property to be projected.</summary>
        /// <remarks>If this node represents an open property or it's the root of the projection tree,
        /// this field is null.</remarks>
        private readonly ResourceProperty property;
        #endregion

        #region Constructors
        /// <summary>Creates new instance of <see cref="ProjectionNode"/> which represents a simple projected property.</summary>
        /// <param name="propertyName">The name of the property to project.</param>
        /// <param name="property">The <see cref="ResourceProperty"/> for the property to project. If an open property
        /// is to be projected, specify null.</param>
        internal ProjectionNode(string propertyName, ResourceProperty property)
        {
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(property == null || property.Name == propertyName, "If the property is specified its name must match.");

            this.propertyName = propertyName;
            this.property = property;
        }
        #endregion

        #region Public properties
        /// <summary>The name of the property to project.</summary>
        /// <remarks>If this node represents the root of the projection tree, this name is an empty string.</remarks>
        public string PropertyName
        {
            get
            {
                return this.propertyName;
            }
        }

        /// <summary>The <see cref="ResourceProperty"/> for the property to be projected.</summary>
        /// <remarks>If this node represents an open property or it's the root of the projection tree,
        /// this property is null.</remarks>
        public ResourceProperty Property
        {
            get
            {
                return this.property;
            }
        }
        #endregion
    }
}
