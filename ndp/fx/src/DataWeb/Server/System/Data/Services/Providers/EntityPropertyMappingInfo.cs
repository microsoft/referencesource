//---------------------------------------------------------------------
// <copyright file="EntityPropertyMappingInfo.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Manages the mapping information for EntityPropertyMappingAttributes
//      on a ResourceType.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System.Diagnostics;
#if ASTORIA_CLIENT
    using System.Data.Services.Client;
    using System.Reflection;
    using ClientTypeOrResourceType_Alias = System.Data.Services.Client.ClientType;
    using TypeOrResourceType_Alias = System.Type;
#else
    using System.Data.Services.Providers;
    using ClientTypeOrResourceType_Alias = System.Data.Services.Providers.ResourceType;
    using TypeOrResourceType_Alias = System.Data.Services.Providers.ResourceType;
#endif

    /// <summary>
    /// Holds information needed during content serialization/deserialization for
    /// each EntityPropertyMappingAttribute
    /// </summary>
    [DebuggerDisplay("EntityPropertyMappingInfo {DefiningType}")]
    internal sealed class EntityPropertyMappingInfo
    {
        /// <summary>
        /// Private field backing Attribute property.
        /// </summary>
        private readonly EntityPropertyMappingAttribute attribute;

        /// <summary>
        /// Private field backing DefiningType property
        /// </summary>
        private readonly TypeOrResourceType_Alias definingType;
        
        /// <summary>
        /// Source property path in the segmented form.. Stored in order not to have to call attribute.SourcePath.Split('/') each time we want to read the property value.
        /// </summary>
        private readonly string[] segmentedSourcePath;

        /// <summary>
        /// Type whose property is to be read. This property is of ClientType type on the client and of ResourceType type on the server.
        /// </summary>
        private readonly ClientTypeOrResourceType_Alias actualPropertyType;

#if !ASTORIA_CLIENT

        /// <summary>
        /// Private field backing IsEFProvider property
        /// </summary>
        private readonly bool isEFProvider;

        /// <summary>
        /// Creates instance of EntityPropertyMappingInfo class.
        /// </summary>
        /// <param name="attribute">The <see cref="EntityPropertyMappingAttribute"/> corresponding to this object</param>
        /// <param name="definingType">Type the <see cref="EntityPropertyMappingAttribute"/> was defined on.</param>
        /// <param name="actualPropertyType">Type whose property is to be read. This can be different from defining type when inheritance is involved.</param>
        /// <param name="isEFProvider">Whether the current data source is an EF provider. Needed for error reporting.</param>
        public EntityPropertyMappingInfo(EntityPropertyMappingAttribute attribute, ResourceType definingType, ResourceType actualPropertyType, bool isEFProvider)
        {
            this.isEFProvider = isEFProvider;
#else
        /// <summary>
        /// Creates instance of EntityPropertyMappingInfo class.
        /// </summary>
        /// <param name="attribute">The <see cref="EntityPropertyMappingAttribute"/> corresponding to this object</param>
        /// <param name="definingType">Type the <see cref="EntityPropertyMappingAttribute"/> was defined on.</param>
        /// <param name="actualPropertyType">ClientType whose property is to be read.</param>
        public EntityPropertyMappingInfo(EntityPropertyMappingAttribute attribute, Type definingType, ClientType actualPropertyType)
        {
#endif
            Debug.Assert(attribute != null, "attribute != null");
            Debug.Assert(definingType != null, "definingType != null");
            Debug.Assert(actualPropertyType != null, "actualPropertyType != null");

            this.attribute = attribute;
            this.definingType = definingType;
            this.actualPropertyType = actualPropertyType;

            Debug.Assert(!string.IsNullOrEmpty(attribute.SourcePath), "Invalid source path");
            this.segmentedSourcePath = attribute.SourcePath.Split('/');
        }

        /// <summary>
        /// The <see cref="EntityPropertyMappingAttribute"/> corresponding to this object
        /// </summary>
        public EntityPropertyMappingAttribute Attribute 
        { 
            get { return this.attribute; }
        }

        /// <summary>
        /// Type that has the <see cref="EntityPropertyMappingAttribute"/>
        /// </summary>
        public TypeOrResourceType_Alias DefiningType
        {
            get { return this.definingType; }
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// Given a source property path reads the property value from the resource type instance
        /// </summary>
        /// <param name="element">Client type instance.</param>
        /// <returns>Property value read from the client type instance. Possibly null.</returns>
        internal object ReadPropertyValue(object element)
        {
            return ReadPropertyValue(element, this.actualPropertyType, this.segmentedSourcePath, 0);
        }

        /// <summary>
        /// Given a source property path in segmented form, reads the property value from the resource type instance
        /// </summary>
        /// <param name="element">Client type instance.</param>
        /// <param name="resourceType">Client type whose property is to be read</param>
        /// <param name="srcPathSegments">Segmented source property path.</param>
        /// <param name="currentSegment">Index of current property name in <paramref name="srcPathSegments"/></param>
        /// <returns>Property value read from the client type instance. Possibly null.</returns>
        private static object ReadPropertyValue(object element, ClientType resourceType, string[] srcPathSegments, int currentSegment)
        {
            if (element == null || currentSegment == srcPathSegments.Length)
            {
                return element;
            }
            else
            {
                String srcPathPart = srcPathSegments[currentSegment];

                ClientType.ClientProperty resourceProperty = resourceType.GetProperty(srcPathPart, true);
                if (resourceProperty == null)
                {
                    throw Error.InvalidOperation(Strings.EpmSourceTree_InaccessiblePropertyOnType(srcPathPart, resourceType.ElementTypeName));
                }

                // If this is the last part of the path, then it has to be a primitive type otherwise should be a complex type
                if (resourceProperty.IsKnownType ^ (currentSegment == srcPathSegments.Length - 1))
                {
                    throw Error.InvalidOperation(!resourceProperty.IsKnownType ? Strings.EpmClientType_PropertyIsComplex(resourceProperty.PropertyName) :
                                                                                 Strings.EpmClientType_PropertyIsPrimitive(resourceProperty.PropertyName));
                }

                // o.Prop
                PropertyInfo pi = element.GetType().GetProperty(srcPathPart, BindingFlags.Instance | BindingFlags.Public);
                Debug.Assert(pi != null, "Cannot find property " + srcPathPart + "on type " + element.GetType().Name);

                return ReadPropertyValue(
                            pi.GetValue(element, null),
                            resourceProperty.IsKnownType ? null : ClientType.Create(resourceProperty.PropertyType),
                            srcPathSegments,
                            ++currentSegment);
            }
        }
#else
        /// <summary>Is the current data source an EF provider</summary>
        public bool IsEFProvider
        {
            get { return this.isEFProvider;  }
        }       

        /// <summary>
        /// Given a source property path reads the property value from the resource type instance.
        /// </summary>
        /// <param name="element">Resource type instance.</param>
        /// <param name="provider">Underlying data provider.</param>
        /// <returns>Property value read from the resource type instance. Possibly null.</returns>
        internal object ReadPropertyValue(object element, DataServiceProviderWrapper provider)
        {
            return ReadPropertyValue(element, provider, this.actualPropertyType, this.segmentedSourcePath, 0);
        }

        /// <summary>
        /// Given a source property path in the segmented form reads the property value from the resource type instance.
        /// </summary>
        /// <param name="element">Resource type instance.</param>
        /// <param name="provider">Underlying data provider.</param>
        /// <param name="resourceType">Resource type whose property is to be read.</param>
        /// <param name="srcPathSegments">Segmented source property path.</param>
        /// <param name="currentSegment">Index of current property name in <paramref name="srcPathSegments"/></param>
        /// <returns>Property value read from the resource type instance. Possibly null.</returns>
        private static object ReadPropertyValue(object element, DataServiceProviderWrapper provider, ResourceType resourceType, string[] srcPathSegments, int currentSegment)
        {
            if (element == null || currentSegment == srcPathSegments.Length)
            {
                return element;
            }
            else
            {
                String propertyName = srcPathSegments[currentSegment];
                ResourceProperty resourceProperty = resourceType != null ? resourceType.TryResolvePropertyName(propertyName) : null;

                if (resourceProperty != null)
                {
                    // If this is the last part of the path, then it has to be a primitive type otherwise should be a complex type
                    if (!resourceProperty.IsOfKind(currentSegment == srcPathSegments.Length - 1 ? ResourcePropertyKind.Primitive : ResourcePropertyKind.ComplexType))
                    {
                        throw new InvalidOperationException(Strings.EpmSourceTree_EndsWithNonPrimitiveType(propertyName));
                    }
                }
                else
                {
                    if (!(resourceType == null || resourceType.IsOpenType))
                    {
                        throw new InvalidOperationException(Strings.EpmSourceTree_InaccessiblePropertyOnType(propertyName, resourceType.Name));
                    }

                    // this is an open type resolve resourceType and try resolving resourceProperty
                    resourceType = WebUtil.GetNonPrimitiveResourceType(provider, element);
                    resourceProperty = resourceType.TryResolvePropertyName(propertyName);
                }

                Debug.Assert(resourceType != null, "resourceType != null");
                object propertyValue = WebUtil.GetPropertyValue(provider, element, resourceType, resourceProperty, resourceProperty == null ? propertyName : null);

                return ReadPropertyValue(
                    propertyValue,
                    provider,
                    resourceProperty != null ? resourceProperty.ResourceType : null,
                    srcPathSegments,
                    currentSegment + 1);
            }
        }
#endif
    }
}
