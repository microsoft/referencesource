//---------------------------------------------------------------------
// <copyright file="ResourceAssociationType.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Simple couple of classes to keep association descriptions
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System.Diagnostics;
    using System.Collections.Generic;

    /// <summary>
    /// Stores information about a association and its ends
    /// </summary>
    internal class ResourceAssociationType
    {
        /// <summary>FullName of the association.</summary>
        private readonly string fullName;

        /// <summary>Name of the association </summary>
        private readonly string name;

        /// <summary>end1 for this association.</summary>
        private readonly ResourceAssociationTypeEnd end1;

        /// <summary>end2 for this association.</summary>
        private readonly ResourceAssociationTypeEnd end2;

        /// <summary>
        /// Creates a new instance of AssociationInfo to store information about an association.
        /// </summary>
        /// <param name="name">name of the association.</param>
        /// <param name="namespaceName">namespaceName of the association.</param>
        /// <param name="end1">first end of the association.</param>
        /// <param name="end2">second end of the association.</param>
        internal ResourceAssociationType(string name, string namespaceName, ResourceAssociationTypeEnd end1, ResourceAssociationTypeEnd end2)
        {
            Debug.Assert(!String.IsNullOrEmpty(name), "!String.IsNullOrEmpty(name)");
            Debug.Assert(end1 != null && end2 != null, "end1 != null && end2 != null");

            this.name = name;
            this.fullName = namespaceName + "." + name;
            this.end1 = end1;
            this.end2 = end2;
        }

        /// <summary>FullName of the association.</summary>
        internal string FullName
        {
            get { return this.fullName; }
        }

        /// <summary>Name of the association.</summary>
        internal string Name
        {
            get { return this.name; }
        }

        /// <summary>end1 for this association.</summary>
        internal ResourceAssociationTypeEnd End1
        {
            get { return this.end1; }
        }

        /// <summary>end2 for this association.</summary>
        internal ResourceAssociationTypeEnd End2
        {
            get { return this.end2; }
        }

        /// <summary>
        /// Retrieve the end for the given resource set, type and property.
        /// </summary>
        /// <param name="resourceType">resource type for the end</param>
        /// <param name="resourceProperty">resource property for the end</param>
        /// <returns>Association type end for the given parameters</returns>
        internal ResourceAssociationTypeEnd GetResourceAssociationTypeEnd(ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceType != null, "resourceType != null");

            foreach (ResourceAssociationTypeEnd end in new[] { this.end1, this.end2 })
            {
                if (end.ResourceType == resourceType && end.ResourceProperty == resourceProperty)
                {
                    return end;
                }
            }

            return null;
        }

        /// <summary>
        /// Retrieve the related end for the given resource set, type and property.
        /// </summary>
        /// <param name="resourceType">resource type for the source end</param>
        /// <param name="resourceProperty">resource property for the source end</param>
        /// <returns>Related association type end for the given parameters</returns>
        internal ResourceAssociationTypeEnd GetRelatedResourceAssociationSetEnd(ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceType != null, "resourceType != null");

            ResourceAssociationTypeEnd thisEnd = this.GetResourceAssociationTypeEnd(resourceType, resourceProperty);

            if (thisEnd != null)
            {
                foreach (ResourceAssociationTypeEnd end in new[] { this.end1, this.end2 })
                {
                    if (end != thisEnd)
                    {
                        return end;
                    }
                }
            }

            return null;
        }
    }
}
