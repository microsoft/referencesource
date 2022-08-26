//---------------------------------------------------------------------
// <copyright file="ResourceAssociationSet.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Describes an association between two resource sets.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System.Diagnostics;
    using System.Collections.Generic;

    /// <summary>
    /// Class to describe an association between two resource sets.
    /// </summary>
    [DebuggerDisplay("ResourceAssociationSet: ({End1.ResourceSet.Name}, {End1.ResourceType.Name}, {End1.ResourceProperty.Name}) <-> ({End2.ResourceSet.Name}, {End2.ResourceType.Name}, {End2.ResourceProperty.Name})")]
    public sealed class ResourceAssociationSet
    {
        #region Private Fields

        /// <summary>
        /// Name of the association set.
        /// </summary>
        private readonly string name;

        /// <summary>
        /// End1 of the association set.
        /// </summary>
        private readonly ResourceAssociationSetEnd end1;

        /// <summary>
        /// End2 of the association set.
        /// </summary>
        private readonly ResourceAssociationSetEnd end2;

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Constructs a resource association set instance.
        /// </summary>
        /// <param name="name">Name of the association set.</param>
        /// <param name="end1">end1 of the association set.</param>
        /// <param name="end2">end2 of the association set.</param>
        public ResourceAssociationSet(string name, ResourceAssociationSetEnd end1, ResourceAssociationSetEnd end2)
        {
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckArgumentNull(end1, "end1");
            WebUtil.CheckArgumentNull(end2, "end2");

            if (end1.ResourceProperty == null && end2.ResourceProperty == null)
            {
                throw new ArgumentException(Strings.ResourceAssociationSet_ResourcePropertyCannotBeBothNull);
            }

            if (end1.ResourceType == end2.ResourceType && end1.ResourceProperty == end2.ResourceProperty)
            {
                throw new ArgumentException(Strings.ResourceAssociationSet_SelfReferencingAssociationCannotBeBiDirectional);
            }

            this.name = name;
            this.end1 = end1;
            this.end2 = end2;
        }

        #endregion Constructor

        #region Properties

        /// <summary>
        /// Name of the association set.
        /// </summary>
        public string Name
        {
            [DebuggerStepThrough]
            get { return this.name; }
        }

        /// <summary>
        /// Source end of the association set.
        /// </summary>
        public ResourceAssociationSetEnd End1
        {
            [DebuggerStepThrough]
            get { return this.end1; }
        }

        /// <summary>
        /// Target end of the association set.
        /// </summary>
        public ResourceAssociationSetEnd End2
        {
            [DebuggerStepThrough]
            get { return this.end2; }
        }

        /// <summary>
        /// Resource association type for the set.
        /// </summary>
        internal ResourceAssociationType ResourceAssociationType
        {
            get;
            set;
        }

        #endregion Properties

        #region Methods

        /// <summary>
        /// Retrieve the end for the given resource set, type and property.
        /// </summary>
        /// <param name="resourceSet">resource set for the end</param>
        /// <param name="resourceType">resource type for the end</param>
        /// <param name="resourceProperty">resource property for the end</param>
        /// <returns>Resource association set end for the given parameters</returns>
        internal ResourceAssociationSetEnd GetResourceAssociationSetEnd(ResourceSetWrapper resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            foreach (ResourceAssociationSetEnd end in new[] { this.end1, this.end2 })
            {
                if (end.ResourceSet.Name == resourceSet.Name && end.ResourceType.IsAssignableFrom(resourceType))
                {
                    if ((end.ResourceProperty == null && resourceProperty == null) ||
                        (end.ResourceProperty != null && resourceProperty != null && end.ResourceProperty.Name == resourceProperty.Name))
                    {
                        return end;
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Retrieve the related end for the given resource set, type and property.
        /// </summary>
        /// <param name="resourceSet">resource set for the source end</param>
        /// <param name="resourceType">resource type for the source end</param>
        /// <param name="resourceProperty">resource property for the source end</param>
        /// <returns>Related resource association set end for the given parameters</returns>
        internal ResourceAssociationSetEnd GetRelatedResourceAssociationSetEnd(ResourceSetWrapper resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");

            ResourceAssociationSetEnd thisEnd = this.GetResourceAssociationSetEnd(resourceSet, resourceType, resourceProperty);

            if (thisEnd != null)
            {
                return thisEnd == this.End1 ? this.End2 : this.End1;
            }

            return null;
        }

        #endregion Methods
    }
}
