//---------------------------------------------------------------------
// <copyright file="ResourceProperty.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to describe properties on resources.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;

    /// <summary>Use this class to describe a property on a resource.</summary>
    [DebuggerDisplay("{kind}: {name}")]
    public class ResourceProperty
    {
        #region Private fields.

        /// <summary>The name of this property.</summary>
        private readonly string name;

        /// <summary>The kind of resource Type that this property refers to.
        /// For e.g. for collection properties, this would return the resource type,
        /// and not the collection type that this property refers to.</summary>
        private readonly ResourceType propertyResourceType;

        /// <summary>The kind of property this is in relation to the resource.</summary>
        private ResourcePropertyKind kind;

        /// <summary> Is true, if this property is a actual clr property on the property type. In this case,
        /// astoria service will do reflection over the property type and get/set value for this property.
        /// False means that astoria service needs to go through the IDataServiceQueryProvider contract to get/set value for this provider.</summary>
        private bool canReflectOnInstanceTypeProperty;

        /// <summary>Is true, if the resource property is set to readonly i.e. fully initialized and validated. No more changes can be made,
        /// once the resource property is set to readonly.</summary>
        private bool isReadOnly;

        /// <summary>MIME type for the property, if it's a primitive value.</summary>
        private string mimeType;

        #endregion Private fields.

        /// <summary>
        /// Initializes a new ResourceProperty instance for an open property.
        /// </summary>
        /// <param name="name">Property name for the property.</param>
        /// <param name="kind">Property kind.</param>
        /// <param name="propertyResourceType">The type of the resource that this property refers to</param>
        public ResourceProperty(
                string name,
                ResourcePropertyKind kind,
                ResourceType propertyResourceType)
        {
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckArgumentNull(propertyResourceType, "propertyResourceType");

            ValidatePropertyParameters(kind, propertyResourceType);

            this.kind = kind;
            this.name = name;
            this.propertyResourceType = propertyResourceType;
            this.canReflectOnInstanceTypeProperty = true;
        }

        #region Properties

        /// <summary>Indicates whether this property can be accessed through reflection on the declaring resource instance type.</summary>
        /// <remarks>A 'true' value here typically indicates astoria service will use reflection to get the property info on the declaring ResourceType.InstanceType.
        /// 'false' means that astoria service will go through IDataServiceQueryProvider interface to get/set this property's value.</remarks>
        public bool CanReflectOnInstanceTypeProperty
        {
            get
            {
                return this.canReflectOnInstanceTypeProperty;
            }

            set
            {
                this.ThrowIfSealed();
                this.canReflectOnInstanceTypeProperty = value;
            }
        }

        /// <summary>
        /// The resource type that is property refers to [For collection, 
        /// this will return the element of the collection, and not the 
        /// collection].
        /// </summary>
        public ResourceType ResourceType
        {
            [DebuggerStepThrough]
            get { return this.propertyResourceType; }
        }

        /// <summary>The property name.</summary>
        public string Name
        {
            [DebuggerStepThrough]
            get { return this.name; }
        }

        /// <summary>MIME type for the property, if it's a primitive value; null if none specified.</summary>
        public string MimeType
        {
            [DebuggerStepThrough]
            get
            {
                return this.mimeType;
            }

            set
            {
                this.ThrowIfSealed();

                if (String.IsNullOrEmpty(value))
                {
                    throw new InvalidOperationException(Strings.ResourceProperty_MimeTypeAttributeEmpty(this.Name));
                }

                if (this.ResourceType.ResourceTypeKind != ResourceTypeKind.Primitive)
                {
                    throw new InvalidOperationException(Strings.ResourceProperty_MimeTypeAttributeOnNonPrimitive(this.Name, this.ResourceType.FullName));
                }

                if (!WebUtil.IsValidMimeType(value))
                {
                    throw new InvalidOperationException(Strings.ResourceProperty_MimeTypeNotValid(value, this.Name));
                }

                this.mimeType = value;
            }
        }

        /// <summary>The kind of property this is in relation to the resource.</summary>
        public ResourcePropertyKind Kind
        {
            [DebuggerStepThrough]
            get
            {
                return this.kind;
            }

            [DebuggerStepThrough]
            internal set
            {
                Debug.Assert(!this.isReadOnly, "Kind - the resource property cannot be readonly");
                this.kind = value;
            }
        }

        /// <summary>
        /// PlaceHolder to hold custom state information about resource property.
        /// </summary>
        public object CustomState
        {
            get;
            set;
        }

        /// <summary>
        /// Returns true, if this resource property has been set to read only. Otherwise returns false.
        /// </summary>
        public bool IsReadOnly
        {
            get { return this.isReadOnly; }
        }

        /// <summary>The kind of type this property has in relation to the data service.</summary>
        internal ResourceTypeKind TypeKind
        {
            get
            {
                return this.ResourceType.ResourceTypeKind;
            }
        }

        /// <summary>The type of the property.</summary>
        internal Type Type
        {
            get
            {
                if (this.Kind == ResourcePropertyKind.ResourceSetReference)
                {
                    return typeof(System.Collections.Generic.IEnumerable<>).MakeGenericType(this.propertyResourceType.InstanceType);
                }
                else
                {
                    return this.propertyResourceType.InstanceType;
                }
            }
        }

        #endregion Properties

        #region Methods
        /// <summary>
        /// Sets the resource property to readonly. Once this method is called, no more changes can be made to resource property.
        /// </summary>
        public void SetReadOnly()
        {
            // If its already set to readonly, do no-op
            if (this.isReadOnly)
            {
                return;
            }

            this.ResourceType.SetReadOnly();
            this.isReadOnly = true;
        }

        /// <summary>
        /// return true if this property is of the given kind
        /// </summary>
        /// <param name="checkKind">flag which needs to be checked on the current property kind</param>
        /// <returns>true if the current property is of the given kind</returns>
        internal bool IsOfKind(ResourcePropertyKind checkKind)
        {
            return ResourceProperty.IsOfKind(this.kind, checkKind);
        }

        /// <summary>
        /// return true if the given property kind is of the given kind
        /// </summary>
        /// <param name="propertyKind">kind of the property</param>
        /// <param name="kind">flag which needs to be checked on property kind</param>
        /// <returns>true if the kind flag is set on the given property kind</returns>
        private static bool IsOfKind(ResourcePropertyKind propertyKind, ResourcePropertyKind kind)
        {
            return ((propertyKind & kind) == kind);
        }

        /// <summary>
        /// Validates that the given property kind is valid
        /// </summary>
        /// <param name="kind">property kind to check</param>
        /// <param name="parameterName">name of the parameter</param>
        private static void CheckResourcePropertyKind(ResourcePropertyKind kind, string parameterName)
        {
            // For open properties, resource property instance is created only for nav properties.
            if (kind != ResourcePropertyKind.ResourceReference &&
                kind != ResourcePropertyKind.ResourceSetReference &&
                kind != ResourcePropertyKind.ComplexType &&
                kind != ResourcePropertyKind.Primitive &&
                kind != (ResourcePropertyKind.Primitive | ResourcePropertyKind.Key) &&
                kind != (ResourcePropertyKind.Primitive | ResourcePropertyKind.ETag))
            {
                throw new ArgumentException(Strings.InvalidEnumValue(kind.GetType().Name), parameterName);
            }
        }

        /// <summary>
        /// Validate the parameters of the resource property constructor.
        /// </summary>
        /// <param name="kind">kind of the resource property.</param>
        /// <param name="propertyResourceType">resource type that this property refers to.</param>
        private static void ValidatePropertyParameters(ResourcePropertyKind kind, ResourceType propertyResourceType)
        {
            CheckResourcePropertyKind(kind, "kind");

            if (IsOfKind(kind, ResourcePropertyKind.ResourceReference) || IsOfKind(kind, ResourcePropertyKind.ResourceSetReference))
            {
                if (propertyResourceType.ResourceTypeKind != ResourceTypeKind.EntityType)
                {
                    throw new ArgumentException(Strings.ResourceProperty_PropertyKindAndResourceTypeKindMismatch("kind", "propertyResourceType"));
                }
            }

            if (IsOfKind(kind, ResourcePropertyKind.Primitive))
            {
                if (propertyResourceType.ResourceTypeKind != ResourceTypeKind.Primitive)
                {
                    throw new ArgumentException(Strings.ResourceProperty_PropertyKindAndResourceTypeKindMismatch("kind", "propertyResourceType"));
                }
            }

            if (IsOfKind(kind, ResourcePropertyKind.ComplexType))
            {
                if (propertyResourceType.ResourceTypeKind != ResourceTypeKind.ComplexType)
                {
                    throw new ArgumentException(Strings.ResourceProperty_PropertyKindAndResourceTypeKindMismatch("kind", "propertyResourceType"));
                }
            }

            if (IsOfKind(kind, ResourcePropertyKind.Key) && Nullable.GetUnderlyingType(propertyResourceType.InstanceType) != null)
            {
                throw new ArgumentException(Strings.ResourceProperty_KeyPropertiesCannotBeNullable);
            }
        }

        /// <summary>
        /// Checks if the resource type is sealed. If not, it throws an InvalidOperationException.
        /// </summary>
        private void ThrowIfSealed()
        {
            if (this.isReadOnly)
            {
                throw new InvalidOperationException(Strings.ResourceProperty_Sealed(this.Name));
            }
        }
        #endregion Methods
    }
}
