//---------------------------------------------------------------------
// <copyright file="ProjectionPathSegment.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that represents a single step in a path of segments
// over a parsed tree used during projection-driven materialization.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Diagnostics;
    using System.Linq.Expressions;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to represent a step in a path of segments
    /// over a parsed tree used during projection-driven materialization.
    /// </summary>
    [DebuggerDisplay("Segment {ProjectionType} {Member}")]
    internal class ProjectionPathSegment
    {
        #region Constructors.

        /// <summary>Initializes a new <see cref="ProjectionPathSegment"/> instance.</summary>
        /// <param name="startPath">Path on which this segment is located.</param>
        /// <param name="member">Name of member to access when traversing a property; possibly null.</param>
        /// <param name="projectionType">
        /// Type that we expect to project out; typically the same as <paramref name="member"/>, but may be adjusted.
        /// </param>
        internal ProjectionPathSegment(ProjectionPath startPath, string member, Type projectionType)
        {
            Debug.Assert(startPath != null, "startPath != null");
            
            this.Member = member;
            this.StartPath = startPath;
            this.ProjectionType = projectionType;
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Name of member to access when traversing a property; possibly null.</summary>
        internal string Member 
        { 
            get; 
            private set; 
        }

        /// <summary>
        /// Type that we expect to project out; typically the same as <propertyref name="Member"/>, but may be adjusted.
        /// </summary>
        /// <remarks>
        /// In particular, this type will be adjusted for nested narrowing entity types.
        /// 
        /// For example:
        /// from c in ctx.Customers select new NarrowCustomer() { 
        ///   ID = c.ID, 
        ///   BestFriend = new NarrowCustomer() { ID = c.BestFriend.ID }
        /// }
        /// 
        /// In this case, ID will match types on both sides, but BestFriend
        /// will be of type Customer in the member access of the source tree
        /// and we want to project out a member-initialized NarrowCustomer
        /// in the target tree.
        /// </remarks>
        internal Type ProjectionType 
        { 
            get; 
            set; 
        }

        /// <summary>Path on which this segment is located.</summary>
        internal ProjectionPath StartPath 
        { 
            get; 
            private set; 
        }

        #endregion Internal properties.
    }
}
