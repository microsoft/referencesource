//---------------------------------------------------------------------
// <copyright file="ResourceAssociationTypeEnd.cs" company="Microsoft">
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
    /// Stores information about an end of an association.
    /// </summary>
    internal class ResourceAssociationTypeEnd
    {
        /// <summary>Name of the relationship end </summary>
        private readonly string name;

        /// <summary>Type of the relationship end.</summary>
        private readonly ResourceType resourceType;

        /// <summary>Property of the relationship end.</summary>
        private readonly ResourceProperty resourceProperty;

        /// <summary>Property on the related end that points to this end. The multiplicity of this end is determined from the fromProperty.</summary>
        private readonly ResourceProperty fromProperty;

        /// <summary>
        /// Creates a new instance of EndInfo.
        /// </summary>
        /// <param name="name">name of the end.</param>
        /// <param name="resourceType">resource type that the end refers to.</param>
        /// <param name="resourceProperty">property of the end.</param>
        /// <param name="fromProperty">Property on the related end that points to this end. The multiplicity of this end is determined from the fromProperty.</param>
        internal ResourceAssociationTypeEnd(string name, ResourceType resourceType, ResourceProperty resourceProperty, ResourceProperty fromProperty)
        {
            Debug.Assert(!String.IsNullOrEmpty(name), "!String.IsNullOrEmpty(name)");
            Debug.Assert(resourceType != null, "type != null");

            this.name = name;
            this.resourceType = resourceType;
            this.resourceProperty = resourceProperty;
            this.fromProperty = fromProperty;
        }

        /// <summary>Name of the relationship end </summary>
        internal string Name
        {
            get { return this.name; }
        }

        /// <summary>Type of the relationship end.</summary>
        internal ResourceType ResourceType
        {
            get { return this.resourceType; }
        }

        /// <summary>Property of the relationship end.</summary>
        internal ResourceProperty ResourceProperty
        {
            get { return this.resourceProperty; }
        }

        /// <summary>Mulitplicity of the relationship end </summary>
        internal string Multiplicity
        {
            get
            {
                if (this.fromProperty != null && this.fromProperty.Kind == ResourcePropertyKind.ResourceReference)
                {
                    return XmlConstants.ZeroOrOne;
                }

                return XmlConstants.Many;
            }
        }
    }
}
