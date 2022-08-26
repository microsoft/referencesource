//---------------------------------------------------------------------
// <copyright file="ResourceAssociationSetEnd.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Describes an end point of a resource association set.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System.Diagnostics;

    /// <summary>
    /// Class to describe an end point of a resource association set.
    /// </summary>
    [DebuggerDisplay("ResourceAssociationSetEnd: {Name}: ({ResourceSet.Name}, {ResourceType.Name}, {ResourceProperty.Name})")]
    public sealed class ResourceAssociationSetEnd
    {
        #region Private Fields

        /// <summary>
        /// Resource set for the association end.
        /// </summary>
        private readonly ResourceSet resourceSet;

        /// <summary>
        /// Resource type for the association end.
        /// </summary>
        private readonly ResourceType resourceType;

        /// <summary>
        /// Resource property for the association end.
        /// </summary>
        private readonly ResourceProperty resourceProperty;

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Constructs a ResourceAssociationEnd instance.
        /// </summary>
        /// <param name="resourceSet">Resource set of the association end.</param>
        /// <param name="resourceType">Resource type of the association end.</param>
        /// <param name="resourceProperty">Resource property of the association end.</param>
        public ResourceAssociationSetEnd(ResourceSet resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            WebUtil.CheckArgumentNull(resourceSet, "resourceSet");
            WebUtil.CheckArgumentNull(resourceType, "resourceType");

            if (resourceProperty != null && (resourceType.TryResolvePropertyName(resourceProperty.Name) == null || resourceProperty.TypeKind != ResourceTypeKind.EntityType))
            {
                throw new ArgumentException(Strings.ResourceAssociationSetEnd_ResourcePropertyMustBeNavigationPropertyOnResourceType);
            }

            if (!resourceSet.ResourceType.IsAssignableFrom(resourceType) && !resourceType.IsAssignableFrom(resourceSet.ResourceType))
            {
                throw new ArgumentException(Strings.ResourceAssociationSetEnd_ResourceTypeMustBeAssignableToResourceSet);
            }

            this.resourceSet = resourceSet;
            this.resourceType = resourceType;

            // Note that for the TargetEnd, resourceProperty can be null.
            this.resourceProperty = resourceProperty;
        }

        #endregion Constructor

        #region Properties

        /// <summary>
        /// Resource set for the association end.
        /// </summary>
        public ResourceSet ResourceSet
        {
            [DebuggerStepThrough]
            get { return this.resourceSet; }
        }

        /// <summary>
        /// Resource type for the association end.
        /// </summary>
        public ResourceType ResourceType
        {
            [DebuggerStepThrough]
            get { return this.resourceType; }
        }

        /// <summary>
        /// Resource property for the association end.
        /// </summary>
        public ResourceProperty ResourceProperty
        {
            [DebuggerStepThrough]
            get { return this.resourceProperty; }
        }

        #endregion Properties
    }
}
