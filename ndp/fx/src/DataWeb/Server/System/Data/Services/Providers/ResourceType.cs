//---------------------------------------------------------------------
// <copyright file="ResourceType.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Contains information about a particular resource.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Data.Services.Common;
    using System.Data.Services.Parsing;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>Use this class to represent a DataService type (primitive, complex or entity).</summary>
    [DebuggerDisplay("{Name}: {InstanceType}, {ResourceTypeKind}")]
    public class ResourceType
    {
        #region Fields.

        /// <summary> empty list of properties </summary>
        internal static readonly ReadOnlyCollection<ResourceProperty> EmptyProperties = new ReadOnlyCollection<ResourceProperty>(new ResourceProperty[0]);

        /// <summary>Primitive string resource type.</summary>
        internal static readonly ResourceType PrimitiveStringResourceType = ResourceType.GetPrimitiveResourceType(typeof(string));

        /// <summary>MethodInfo for object DataServiceProviderWrapper.GetPropertyValue(object target, ResourceProperty resourceProperty, ResourceType resourceType).</summary>
        private static readonly MethodInfo GetPropertyValueMethodInfo = typeof(DataServiceProviderWrapper).GetMethod(
            "GetPropertyValue",
            WebUtil.PublicInstanceBindingFlags);

        /// <summary>MethodInfo for object IProjectedResult.GetProjectedPropertyValue(this IProjectedResult value, string propertyName).</summary>
        private static readonly MethodInfo IProjectedResultGetProjectedPropertyValueMethodInfo = typeof(IProjectedResult).GetMethod(
            "GetProjectedPropertyValue",
            WebUtil.PublicInstanceBindingFlags);

        /// <summary> ResourceTypeKind for the type that this structure represents </summary>
        private readonly ResourceTypeKind resourceTypeKind;

        /// <summary> Reference to clr type that this resource represents </summary>
        private readonly Type type;

        /// <summary> Reference to base resource type </summary>
        private readonly ResourceType baseType;

        /// <summary> name of the resource.</summary>
        private readonly string name;

        /// <summary> full name of the resource.</summary>
        private readonly string fullName;

        /// <summary> Namespace for this type.</summary>
        private readonly string namespaceName;

        /// <summary>Whether this type is abstract.</summary>
        private readonly bool abstractType;

        /// <summary>Whether the resource type has open properties.</summary>
        private bool isOpenType;

        /// <summary>Whether the corresponding instance type actually represents this node's CLR type.</summary>
        private bool canReflectOnInstanceType;

        /// <summary>Cached delegate to create a new instance of this type.</summary>
        private Func<object> constructorDelegate;

        /// <summary>Cached delegate to serialize parts of this resource into a dictionary.</summary>
        private Action<object, System.Data.Services.Serializers.DictionaryContent> dictionarySerializerDelegate;

        /// <summary> List of properties declared in this type (includes properties only defined in this type, not in the base type) </summary>
        private IList<ResourceProperty> propertiesDeclaredOnThisType;

        /// <summary> List of all properties for this type (includes properties defined in the base type also) </summary>
        private ReadOnlyCollection<ResourceProperty> allProperties;

        /// <summary> list of key properties for this type</summary>
        private ReadOnlyCollection<ResourceProperty> keyProperties;

        /// <summary> list of etag properties for this type.</summary>
        private ReadOnlyCollection<ResourceProperty> etagProperties;

        /// <summary>If ResourceProperty.CanReflectOnInstanceTypeProperty is true, we cache the PropertyInfo object.</summary>
        private Dictionary<ResourceProperty, PropertyInfo> propertyInfosDeclaredOnThisType = new Dictionary<ResourceProperty, PropertyInfo>(ReferenceEqualityComparer<ResourceProperty>.Instance);

        /// <summary>EpmInfo for this <see cref="ResourceType"/></summary>
        private EpmInfoPerResourceType epmInfo;

        /// <summary>Indicates whether one of the base class of this resource type has EpmInfo.</summary>
        private bool? basesHaveEpmInfo;

        /// <summary>is true, if the type is set to readonly.</summary>
        private bool isReadOnly;

        /// <summary>True if the resource type includes a default stream </summary>
        private bool isMediaLinkEntry;

        /// <summary>True if the virtual load properties is already called, otherwise false.</summary>
        private bool isLoadPropertiesMethodCalled;

        #endregion Fields.

        #region Constructors.

        /// <summary>
        /// Constructs a new instance of Astoria type using the specified clr type
        /// </summary>
        /// <param name="instanceType">clr type that represents the flow format inside the Astoria runtime</param>
        /// <param name="resourceTypeKind"> kind of the resource type</param>
        /// <param name="baseType">base type of the resource type</param>
        /// <param name="namespaceName">Namespace name of the given resource type.</param>
        /// <param name="name">name of the given resource type.</param>
        /// <param name="isAbstract">whether the resource type is an abstract type or not.</param>
        public ResourceType(
                    Type instanceType,
                    ResourceTypeKind resourceTypeKind,
                    ResourceType baseType,
                    string namespaceName,
                    string name,
                    bool isAbstract)
            : this(instanceType, baseType, namespaceName, name, isAbstract)
        {
            WebUtil.CheckArgumentNull(instanceType, "instanceType");
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckResourceTypeKind(resourceTypeKind, "resourceTypeKind");
            if (resourceTypeKind == ResourceTypeKind.Primitive)
            {
                throw new ArgumentException(Strings.ResourceType_InvalidValueForResourceTypeKind, "resourceTypeKind");
            }

            if (instanceType.IsValueType)
            {
                throw new ArgumentException(Strings.ResourceType_TypeCannotBeValueType, "instanceType");
            }

            this.resourceTypeKind = resourceTypeKind;
        }

        /// <summary>
        /// Constructs a new instance of Resource type for the given clr primitive type. This constructor must be called only for primitive types.
        /// </summary>
        /// <param name="type">clr type representing the primitive type.</param>
        /// <param name="namespaceName">namespace of the primitive type.</param>
        /// <param name="name">name of the primitive type.</param>
        internal ResourceType(Type type, string namespaceName, string name)
            : this(type, null, namespaceName, name, false)
        {
            Debug.Assert(WebUtil.IsPrimitiveType(type), "This constructor should be called only for primitive types");
            this.resourceTypeKind = ResourceTypeKind.Primitive;
            this.isReadOnly = true;
        }

        /// <summary>
        /// Constructs a new instance of Astoria type using the specified clr type
        /// </summary>
        /// <param name="type">clr type from which metadata needs to be pulled </param>
        /// <param name="baseType">base type of the resource type</param>
        /// <param name="namespaceName">Namespace name of the given resource type.</param>
        /// <param name="name">name of the given resource type.</param>
        /// <param name="isAbstract">whether the resource type is an abstract type or not.</param>
        private ResourceType(
                    Type type,
                    ResourceType baseType,
                    string namespaceName,
                    string name,
                    bool isAbstract)
        {
            WebUtil.CheckArgumentNull(type, "type");
            WebUtil.CheckArgumentNull(name, "name");

            this.name = name;
            this.namespaceName = namespaceName ?? string.Empty;

            // This is to optimize the string property name in PlainXmlSerializer.WriteStartElementWithType function.
            // Checking here is a fixed overhead, and the gain is every time we serialize a string property.
            if (name == "String" && Object.ReferenceEquals(namespaceName, XmlConstants.EdmNamespace))
            {
                this.fullName = XmlConstants.EdmStringTypeName;
            }
            else
            {
                this.fullName = string.IsNullOrEmpty(namespaceName) ? name : namespaceName + "." + name;
            }

            this.type = type;
            this.abstractType = isAbstract;
            this.canReflectOnInstanceType = true;

            if (baseType != null)
            {
                this.baseType = baseType;
            }
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>True if the resource type includes a default stream</summary>
        public bool IsMediaLinkEntry
        {
            [DebuggerStepThrough]
            get
            {
                return this.isMediaLinkEntry;
            }

            set
            {
                this.ThrowIfSealed();
                if (this.resourceTypeKind != ResourceTypeKind.EntityType && value == true)
                {
                    throw new InvalidOperationException(Strings.ReflectionProvider_HasStreamAttributeOnlyAppliesToEntityType(this.name));
                }

                this.isMediaLinkEntry = value;
            }
        }

        /// <summary> Reference to clr type that this resource represents </summary>
        public Type InstanceType
        {
            [DebuggerStepThrough]
            get { return this.type; }
        }

        /// <summary> Reference to base resource type, if any </summary>
        public ResourceType BaseType
        {
            [DebuggerStepThrough]
            get { return this.baseType; }
        }

        /// <summary> ResourceTypeKind of this type </summary>
        public ResourceTypeKind ResourceTypeKind
        {
            [DebuggerStepThrough]
            get { return this.resourceTypeKind; }
        }

        /// <summary> Returns the list of properties for this type </summary>
        public ReadOnlyCollection<ResourceProperty> Properties
        {
            get
            {
                return this.InitializeProperties();
            }
        }

        /// <summary> list of properties declared on this type </summary>
        public ReadOnlyCollection<ResourceProperty> PropertiesDeclaredOnThisType
        {
            get
            {
                ReadOnlyCollection<ResourceProperty> readOnlyProperties = this.propertiesDeclaredOnThisType as ReadOnlyCollection<ResourceProperty>;
                if (readOnlyProperties == null)
                {
                    // This method will call the virtual method, if that's not been called yet and add the list of properties
                    // returned by the virtual method to the properties collection.
                    this.GetPropertiesDeclaredOnThisType();
                    readOnlyProperties = new ReadOnlyCollection<ResourceProperty>(this.propertiesDeclaredOnThisType ?? ResourceType.EmptyProperties);

                    if (!this.isReadOnly)
                    {
                        return readOnlyProperties;
                    }

                    // First try and validate the type. If that succeeds, then cache the results. otherwise we need to revert the results.
                    IList<ResourceProperty> propertyCollection = this.propertiesDeclaredOnThisType;
                    this.propertiesDeclaredOnThisType = readOnlyProperties;

                    try
                    {
                        this.ValidateType();
                    }
                    catch (Exception)
                    {
                        this.propertiesDeclaredOnThisType = propertyCollection;
                        throw;
                    }
                }

                Debug.Assert(this.isReadOnly, "PropetiesDeclaredInThisType - at this point, the resource type must be readonly");
                return readOnlyProperties;
            }
        }

        /// <summary> Returns the list of key properties for this type, if this type is entity type.</summary>
        public ReadOnlyCollection<ResourceProperty> KeyProperties
        {
            get
            {
                if (this.keyProperties == null)
                {
                    ResourceType rootType = this;
                    while (rootType.BaseType != null)
                    {
                        rootType = rootType.BaseType;
                    }

                    ReadOnlyCollection<ResourceProperty> readOnlyKeyProperties;
                    if (rootType.Properties == null)
                    {
                        readOnlyKeyProperties = ResourceType.EmptyProperties;
                    }
                    else
                    {
                        List<ResourceProperty> key = rootType.Properties.Where(p => p.IsOfKind(ResourcePropertyKind.Key)).ToList();
                        key.Sort(ResourceType.ResourcePropertyComparison);
                        readOnlyKeyProperties = new ReadOnlyCollection<ResourceProperty>(key);
                    }

                    if (!this.isReadOnly)
                    {
                        return readOnlyKeyProperties;
                    }

                    this.keyProperties = readOnlyKeyProperties;
                }

                Debug.Assert(this.isReadOnly, "KeyProperties - at this point, the resource type must be readonly");
                Debug.Assert(
                    (this.ResourceTypeKind != ResourceTypeKind.EntityType && this.keyProperties.Count == 0) ||
                    (this.ResourceTypeKind == ResourceTypeKind.EntityType && this.keyProperties.Count > 0),
                    "Entity type must have key properties and non-entity types cannot have key properties");

                return this.keyProperties;
            }
        }

        /// <summary>Returns the list of etag properties for this type.</summary>
        public ReadOnlyCollection<ResourceProperty> ETagProperties
        {
            get
            {
                if (this.etagProperties == null)
                {
                    ReadOnlyCollection<ResourceProperty> etag = new ReadOnlyCollection<ResourceProperty>(this.Properties.Where(p => p.IsOfKind(ResourcePropertyKind.ETag)).ToList());
                    if (!this.isReadOnly)
                    {
                        return etag;
                    }

                    this.etagProperties = etag;
                }

                Debug.Assert(this.isReadOnly, "ETagProperties - at this point, the resource type must be readonly");
                return this.etagProperties;
            }
        }

        /// <summary> Gets the name of the resource.</summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary> Gets the fullname of the resource.</summary>
        public string FullName
        {
            get { return this.fullName; }
        }

        /// <summary> Returns the namespace of this type.</summary>
        public string Namespace
        {
            get { return this.namespaceName; }
        }

        /// <summary>Indicates whether this is an abstract type.</summary>
        public bool IsAbstract
        {
            get { return this.abstractType; }
        }

        /// <summary>Indicates whether the resource type has open properties.</summary>
        public bool IsOpenType
        {
            [DebuggerStepThrough]
            get
            {
                return this.isOpenType;
            }

            set
            {
                this.ThrowIfSealed();

                // Complex types can not be marked as open.
                if (this.resourceTypeKind == ResourceTypeKind.ComplexType && value == true)
                {
                    throw new InvalidOperationException(Strings.ResourceType_ComplexTypeCannotBeOpen(this.FullName));
                }

                this.isOpenType = value;
            }
        }

        /// <summary>Whether the corresponding instance type actually represents this node's CLR type.</summary>
        public bool CanReflectOnInstanceType
        {
            [DebuggerStepThrough]
            get
            {
                return this.canReflectOnInstanceType;
            }

            set
            {
                this.ThrowIfSealed();
                this.canReflectOnInstanceType = value;
            }
        }

        /// <summary>
        /// PlaceHolder to hold custom state information about resource type.
        /// </summary>
        public object CustomState
        {
            get;
            set;
        }

        /// <summary>
        /// Returns true, if this resource type has been set to read only. Otherwise returns false.
        /// </summary>
        public bool IsReadOnly
        {
            get { return this.isReadOnly; }
        }

        /// <summary>Cached delegate to create a new instance of this type.</summary>
        internal Func<object> ConstructorDelegate
        {
            get
            {
                if (this.constructorDelegate == null)
                {
                    this.constructorDelegate = (Func<object>)
                        WebUtil.CreateNewInstanceConstructor(this.InstanceType, this.FullName, typeof(object));
                }

                return this.constructorDelegate;
            }
        }

        /// <summary>Cached delegate to serialize parts of this resource into a dictionary.</summary>
        internal Action<object, System.Data.Services.Serializers.DictionaryContent> DictionarySerializerDelegate
        {
            get { return this.dictionarySerializerDelegate; }
            set { this.dictionarySerializerDelegate = value; }
        }

        /// <summary>
        /// Do we have entity property mappings for this <see cref="ResourceType"/>
        /// </summary>
        internal bool HasEntityPropertyMappings
        {
            get
            {
                Debug.Assert(this.IsReadOnly, "Type must be read-only.");
    
                if (this.epmInfo != null)
                {
                    return true;
                }

                if (this.basesHaveEpmInfo == null)
                {
                    this.basesHaveEpmInfo = this.BaseType != null ? this.BaseType.HasEntityPropertyMappings  : false;
                }

                return this.basesHaveEpmInfo.Value;
            }
        }

        /// <summary>
        /// Property used to mark the fact that EpmInfo for the resource type has been initialized
        /// </summary>
        internal bool EpmInfoInitialized
        {
            get;
            set;
        }

        /// <summary>The mappings for friendly feeds are V1 compatible or not</summary>
        internal bool EpmIsV1Compatible
        {
            get
            {
                Debug.Assert(this.isReadOnly, "Resource type must already be read-only.");
                this.InitializeProperties();
                return !this.HasEntityPropertyMappings || this.EpmTargetTree.IsV1Compatible;
            }
        }

        /// <summary>
        /// Tree of source paths for EntityPropertyMappingAttributes on this resource type
        /// </summary>
        internal EpmSourceTree EpmSourceTree
        {
            get
            {
                if (this.epmInfo == null)
                {
                    this.epmInfo = new EpmInfoPerResourceType();
                }

                return this.epmInfo.EpmSourceTree;
            }
        }

        /// <summary>
        /// Tree of target paths for EntityPropertyMappingAttributes on this resource type
        /// </summary>
        internal EpmTargetTree EpmTargetTree
        {
            get
            {
                Debug.Assert(this.epmInfo != null, "Must have valid EpmInfo");
                return this.epmInfo.EpmTargetTree;
            }
        }

        /// <summary>Inherited EpmInfo</summary>
        internal IList<EntityPropertyMappingAttribute> InheritedEpmInfo
        {
            get
            {
                Debug.Assert(this.epmInfo != null, "Must have valid EpmInfo");
                return this.epmInfo.InheritedEpmInfo;
            }
        }

        /// <summary>Own EpmInfo</summary>
        internal IList<EntityPropertyMappingAttribute> OwnEpmInfo
        {
            get
            {
                Debug.Assert(this.epmInfo != null, "Must have valid EpmInfo");
                return this.epmInfo.OwnEpmInfo;
            }
        }

        /// <summary>All EpmInfo i.e. both own and inherited.</summary>
        internal IEnumerable<EntityPropertyMappingAttribute> AllEpmInfo
        {
            get
            {
                Debug.Assert(this.epmInfo != null, "Must have valid EpmInfo");
                return this.epmInfo.OwnEpmInfo.Concat(this.epmInfo.InheritedEpmInfo);
            }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>
        /// Get a ResourceType representing a primitive type given a .NET System.Type object
        /// </summary>
        /// <param name="type">.NET type to get the primitive type from</param>
        /// <returns>A ResourceType object representing the primitive type or null if not primitive</returns>
        public static ResourceType GetPrimitiveResourceType(Type type)
        {
            WebUtil.CheckArgumentNull(type, "type");

            foreach (ResourceType resourceType in WebUtil.GetPrimitiveTypes())
            {
                if (resourceType.InstanceType == type)
                {
                    return resourceType;
                }
            }

            return null;
        }

        /// <summary>
        /// Adds the given property to this ResourceType instance
        /// </summary>
        /// <param name="property">resource property to be added</param>
        public void AddProperty(ResourceProperty property)
        {
            WebUtil.CheckArgumentNull(property, "property");

            // only check whether the property with the same name exists in this type.
            // we will look in base types properties when the type is sealed.
            this.ThrowIfSealed();

            // add the property to the list of properties declared on this type.
            this.AddPropertyInternal(property);
        }

        /// <summary>
        /// Adds an <see cref="EntityPropertyMappingAttribute"/> for the resource type.
        /// </summary>
        /// <param name="attribute">Given <see cref="EntityPropertyMappingAttribute"/></param>
        public void AddEntityPropertyMappingAttribute(EntityPropertyMappingAttribute attribute)
        {
            WebUtil.CheckArgumentNull(attribute, "attribute");

            // EntityPropertyMapping attribute can not be added to readonly resource types.
            this.ThrowIfSealed();

            if (this.ResourceTypeKind != ResourceTypeKind.EntityType)
            {
                throw new InvalidOperationException(Strings.EpmOnlyAllowedOnEntityTypes(this.Name));
            }

            if (this.epmInfo == null)
            {
                this.epmInfo = new EpmInfoPerResourceType();
            }

            this.OwnEpmInfo.Add(attribute);
        }

        /// <summary>
        /// Make the resource type readonly from now on. This means that no more changes can be made to the resource type anymore.
        /// </summary>
        public void SetReadOnly()
        {
#if DEBUG
            IList<ResourceProperty> currentPropertyCollection = this.propertiesDeclaredOnThisType;
#endif
            // if its already sealed, its a no-op
            if (this.isReadOnly)
            {
                return;
            }

            // We need to set readonly at the start to avoid any circular loops that may result due to navigation properties.
            // If there are any exceptions, we need to set readonly to false.
            this.isReadOnly = true;

            // There can be properties with the same name in the base class also (using the new construct)
            // if the base type is not null, then we need to make sure that there is no property with the same name.
            // Otherwise, we are only populating property declared for this type and clr gaurantees that they are unique
            if (this.BaseType != null)
            {
                this.BaseType.SetReadOnly();

                // Mark current type as OpenType if base is an OpenType
                if (this.BaseType.IsOpenType && this.ResourceTypeKind != ResourceTypeKind.ComplexType)
                {
                    this.isOpenType = true;
                }

                // Mark the current type as being a Media Link Entry if the base type is a Media Link Entry.
                if (this.BaseType.IsMediaLinkEntry) 
                {
                    this.isMediaLinkEntry = true;
                }

                // Make sure current type is not a CLR type if base is not a CLR type.
                if (!this.BaseType.CanReflectOnInstanceType)
                {
                    this.canReflectOnInstanceType = false;
                }
            }

            // set all the properties to readonly
            if (this.propertiesDeclaredOnThisType != null)
            {
                foreach (ResourceProperty p in this.propertiesDeclaredOnThisType)
                {
                    p.SetReadOnly();
                }
            }
#if DEBUG
            // We cannot change the properties collection method. Basically, we should not be calling Properties or PropertiesDeclaredOnThisType properties
            // since they call the virtual LoadPropertiesDeclaredOnThisType and we want to postpone that virtual call until we actually need to do something
            // more useful with the properties
            Debug.Assert(Object.ReferenceEquals(this.propertiesDeclaredOnThisType, currentPropertyCollection), "We should not have modified the properties collection instance");
#endif
        }

        /// <summary>By initializing the EpmInfo for the resource type, ensures that the information is available for de-serialization.</summary>
        internal void EnsureEpmInfoAvailability()
        {
            this.InitializeProperties();
        }

        /// <summary>Given a resource type, builds the EntityPropertyMappingInfo for each EntityPropertyMappingAttribute on it</summary>
        /// <param name="currentResourceType">Resouce type for which EntityPropertyMappingAttribute discovery is happening</param>
        internal void BuildReflectionEpmInfo(ResourceType currentResourceType)
        {
            if (currentResourceType.BaseType != null)
            {
                this.BuildReflectionEpmInfo(currentResourceType.BaseType);
            }

            foreach (EntityPropertyMappingAttribute epmAttr in currentResourceType.InstanceType.GetCustomAttributes(typeof(EntityPropertyMappingAttribute), currentResourceType.BaseType != null ? false : true))
            {
                this.BuildEpmInfo(epmAttr, currentResourceType, false);

                if (this == currentResourceType)
                {
                    if (!this.PropertyExistsInCurrentType(epmAttr))
                    {
                        this.InheritedEpmInfo.Add(epmAttr);
                    }
                    else
                    {
                        this.OwnEpmInfo.Add(epmAttr);
                    }
                }
            }
        }

        /// <summary>
        /// Builds the EntityPropertyMappingInfo corresponding to an EntityPropertyMappingAttribute, also builds the delegate to
        /// be invoked in order to retrieve the property provided in the <paramref name="epmAttr"/>
        /// </summary>
        /// <param name="epmAttr">Source EntityPropertyMappingAttribute</param>
        /// <param name="definingType">Type that has the attribute applied to it</param>
        /// <param name="isEFProvider">Is EF provider being initialized, used for error message formatting</param>
        internal void BuildEpmInfo(EntityPropertyMappingAttribute epmAttr, ResourceType definingType, bool isEFProvider)
        {
            this.EpmSourceTree.Add(new EntityPropertyMappingInfo(epmAttr, definingType, this, isEFProvider));
        }

        /// <summary>
        /// Sets the value <paramref name="propertyValue"/> on the <paramref name="currentValue"/> object
        /// </summary>
        /// <param name="currentSegment">Target path segment containing the corresponding attribute information</param>
        /// <param name="currentValue">Object on which to set property</param>
        /// <param name="propertyValue">Value to be set</param>
        /// <param name="deserializer">Current deserializer</param>
        internal void SetEpmValue(EpmTargetPathSegment currentSegment, Object currentValue, object propertyValue, EpmContentDeSerializerBase deserializer)
        {
            if (currentSegment.EpmInfo.Attribute.KeepInContent == false)
            {
                this.SetPropertyValueFromPath(
                        currentSegment.EpmInfo.Attribute.SourcePath.Split('/'),
                        this,
                        currentValue,
                        propertyValue,
                        0,
                        deserializer);
            }
        }

        /// <summary>
        /// Given a collection of <paramref name="segments"/> corresponding to a property access path
        /// on the <paramref name="element"/> object, sets the <paramref name="propertyValue"/> on the property
        /// </summary>
        /// <param name="segments">Property access path where each element is a property name</param>
        /// <param name="resourceType">Resource type for which to set the property</param>
        /// <param name="element">Object on which to set property</param>
        /// <param name="propertyValue">Value of property</param>
        /// <param name="currentIndex">Index of the current segment being looked at</param>
        /// <param name="deserializer">Current deserializer</param>
        internal void SetPropertyValueFromPath(
            String[] segments,
            ResourceType resourceType,
            object element,
            object propertyValue,
            int currentIndex,
            EpmContentDeSerializerBase deserializer)
        {
            String currentSegment = segments[currentIndex];

            ResourceProperty clientProp = resourceType.TryResolvePropertyName(currentSegment);
            ResourceType propertyType;
            if (clientProp == null && resourceType.IsOpenType == false)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(currentSegment, resourceType.FullName));
            }

            // If this is a open property OR we do not have to do type conversion for primitive types,
            // read the type from the payload.
            if (clientProp == null ||
                (!deserializer.Service.Configuration.EnableTypeConversion && clientProp.ResourceType.ResourceTypeKind == ResourceTypeKind.Primitive))
            {
                String foundTypeName = deserializer.PropertiesApplied.MapPropertyToType(String.Join("/", segments, 0, currentIndex + 1));
                if (foundTypeName != null)
                {
                    propertyType = WebUtil.TryResolveResourceType(deserializer.Service.Provider, foundTypeName);
                    if (propertyType == null)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeName(foundTypeName));
                    }

                    if (propertyType.ResourceTypeKind == ResourceTypeKind.EntityType)
                    {
                        throw DataServiceException.CreateBadRequestError(
                            Strings.PlainXml_EntityTypeNotSupported(propertyType.FullName));
                    }
                }
                else
                {
                    propertyType = ResourceType.PrimitiveStringResourceType;
                }
            }
            else
            {
                propertyType = clientProp.ResourceType;
            }

            object currentValue;

            // Re-construct the source path to add the newly applied property
            string sourcePath = string.Join("/", segments, 0, currentIndex + 1);

            switch (propertyType.ResourceTypeKind)
            {
                case ResourceTypeKind.ComplexType:
                    if (!deserializer.PropertiesApplied.Lookup(sourcePath))
                    {
                        // Complex types are treated as atomic and we never allow merging of properties belonging to
                        // a complex type.  In other words, we either update the whole complex type or not at all, 
                        // never just a subset of its properties.  If the complex property has not been applied yet
                        // we create a new instance then apply its property mappings.
                        currentValue = deserializer.Updatable.CreateResource(null, propertyType.FullName);
                        ResourceType.SetEpmProperty(element, currentSegment, currentValue, sourcePath, deserializer);
                    }
                    else
                    {
                        // We've already created a new instance of the complex property by now, reuse the same instance.
                        currentValue = deserializer.Updatable.GetValue(element, currentSegment);
                        Debug.Assert(currentValue != null, "currentValue != null -- we should never be here if the complex property were null.");
                    }

                    this.SetPropertyValueFromPath(segments, propertyType, currentValue, propertyValue, ++currentIndex, deserializer);
                    break;
                case ResourceTypeKind.EntityType:
                    throw DataServiceException.CreateBadRequestError(
                        Strings.PlainXml_NavigationPropertyNotSupported(clientProp.Name));
                default:
                    Debug.Assert(
                        propertyType.ResourceTypeKind == ResourceTypeKind.Primitive,
                        "property.TypeKind == ResourceTypeKind.Primitive -- metadata shouldn't return " + propertyType.ResourceTypeKind);

                    currentValue = PlainXmlDeserializer.ConvertValuesForXml(propertyValue, currentSegment, propertyType.InstanceType);

                    // Do not try to update the property if it is a key property
                    if (!deserializer.IsUpdateOperation || clientProp == null || !clientProp.IsOfKind(ResourcePropertyKind.Key))
                    {
                        ResourceType.SetEpmProperty(element, currentSegment, currentValue, sourcePath, deserializer);
                    }

                    break;
            }
        }

        /// <summary>
        /// Changes the key property to non key property and removes it from the key properties list
        /// </summary>
        internal void RemoveKeyProperties()
        {
            Debug.Assert(!this.isReadOnly, "The resource type cannot be sealed - RemoveKeyProperties");
            ReadOnlyCollection<ResourceProperty> key = this.KeyProperties;

            Debug.Assert(key.Count == 1, "Key Properties count must be zero");
            Debug.Assert(this.BaseType == null, "BaseType must be null");
            Debug.Assert(key[0].IsOfKind(ResourcePropertyKind.Key), "must be key property");

            ResourceProperty property = key[0];
            property.Kind = property.Kind ^ ResourcePropertyKind.Key;
        }

        /// <summary>Tries to find the property for the specified name.</summary>
        /// <param name="propertyName">Name of property to resolve.</param>
        /// <returns>Resolved property; possibly null.</returns>
        internal ResourceProperty TryResolvePropertyName(string propertyName)
        {
            // In case of empty property name this will return null, which means propery is not found
            return this.Properties.FirstOrDefault(p => p.Name == propertyName);
        }

        /// <summary>Tries to find the property declared on this type for the specified name.</summary>
        /// <param name="propertyName">Name of property to resolve.</param>
        /// <returns>Resolved property; possibly null.</returns>
        internal ResourceProperty TryResolvePropertiesDeclaredOnThisTypeByName(string propertyName)
        {
            // In case of empty property name this will return null, which means propery is not found
            return this.PropertiesDeclaredOnThisType.FirstOrDefault(p => p.Name == propertyName);
        }

        /// <summary>
        /// Checks if the given type is assignable to this type. In other words, if this type
        /// is a subtype of the given type or not.
        /// </summary>
        /// <param name="superType">resource type to check.</param>
        /// <returns>true, if the given type is assignable to this type. Otherwise returns false.</returns>
        internal bool IsAssignableFrom(ResourceType superType)
        {
            while (superType != null)
            {
                if (superType == this)
                {
                    return true;
                }

                superType = superType.BaseType;
            }

            return false;
        }

        /// <summary>
        /// Gets the property info for the resource property
        /// </summary>
        /// <param name="resourceProperty">Resource property instance to get the property info</param>
        /// <returns>Returns the propertyinfo object for the specified resource property.</returns>
        /// <remarks>The method searchies this type as well as all its base types for the property.</remarks>
        internal PropertyInfo GetPropertyInfo(ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            Debug.Assert(resourceProperty.CanReflectOnInstanceTypeProperty, "resourceProperty.CanReflectOnInstanceTypeProperty");

            PropertyInfo propertyInfo = null;
            ResourceType resourceType = this;
            while (propertyInfo == null && resourceType != null)
            {
                propertyInfo = resourceType.GetPropertyInfoDecaredOnThisType(resourceProperty);
                resourceType = resourceType.BaseType;
            }

            Debug.Assert(propertyInfo != null, "propertyInfo != null");
            return propertyInfo;
        }

        /// <summary>Sets the value of the property.</summary>
        /// <param name="instance">The object whose property needs to be set.</param>
        /// <param name="propertyValue">new value for the property.</param>
        /// <param name="resourceProperty">metadata for the property to be set.</param>
        internal void SetValue(object instance, object propertyValue, ResourceProperty resourceProperty)
        {
            Debug.Assert(instance != null, "instance != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");

            MethodInfo setMethod = this.GetPropertyInfo(resourceProperty).GetSetMethod();
            if (setMethod == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_PropertyValueCannotBeSet(resourceProperty.Name));
            }

            try
            {
                setMethod.Invoke(instance, new object[] { propertyValue });
            }
            catch (TargetInvocationException exception)
            {
                ErrorHandler.HandleTargetInvocationException(exception);
                throw;
            }
            catch (ArgumentException exception)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInSettingPropertyValue(resourceProperty.Name), exception);
            }
        }

        /// <summary>
        /// Return the list of properties declared by this resource type. This method gives a chance to lazy load the properties
        /// of a resource type, instead of loading them upfront. This property will be called once and only once, whenever
        /// ResourceType.Properties or ResourceType.PropertiesDeclaredOnThisType property is accessed.
        /// </summary>
        /// <returns>the list of properties declared on this type.</returns>
        protected virtual IEnumerable<ResourceProperty> LoadPropertiesDeclaredOnThisType()
        {
            return new ResourceProperty[0];
        }

        /// <summary>
        /// Compares two resource property instances, sorting them so keys are first,
        /// and are alphabetically ordered in case-insensitive ordinal order.
        /// </summary>
        /// <param name="a">First property to compare.</param>
        /// <param name="b">Second property to compare.</param>
        /// <returns>
        /// Less than zero if a sorts before b; zero if equal; greater than zero if a sorts
        /// after b.
        /// </returns>
        private static int ResourcePropertyComparison(ResourceProperty a, ResourceProperty b)
        {
            return StringComparer.OrdinalIgnoreCase.Compare(a.Name, b.Name);
        }

        /// <summary>
        /// Sets a mapped property value and mark its source path as applied
        /// </summary>
        /// <param name="element">Object on which to set the property</param>
        /// <param name="propertyName">Name of the property</param>
        /// <param name="propertyValue">Value of the property</param>
        /// <param name="sourcePath">Source mapping path for the property to be set</param>
        /// <param name="deserializer">Current deserializer</param>
        private static void SetEpmProperty(object element, string propertyName, object propertyValue, string sourcePath, EpmContentDeSerializerBase deserializer)
        {
            deserializer.Updatable.SetValue(element, propertyName, propertyValue);
            deserializer.PropertiesApplied.AddAppliedProperty(sourcePath, false);
        }

        /// <summary>
        /// Initializes all properties for the resource type, to be used by Properties getter.
        /// </summary>
        /// <returns>Collection of properties exposed by this resource type.</returns>
        private ReadOnlyCollection<ResourceProperty> InitializeProperties()
        {
            if (this.allProperties == null)
            {
                ReadOnlyCollection<ResourceProperty> readOnlyAllProps;
                List<ResourceProperty> allProps = new List<ResourceProperty>();
                if (this.BaseType != null)
                {
                    allProps.AddRange(this.BaseType.Properties);
                }

                allProps.AddRange(this.PropertiesDeclaredOnThisType);
                readOnlyAllProps = new ReadOnlyCollection<ResourceProperty>(allProps);

                if (!this.isReadOnly)
                {
                    return readOnlyAllProps;
                }

                this.allProperties = readOnlyAllProps;
            }

            Debug.Assert(this.isReadOnly, "Propeties - at this point, the resource type must be readonly");
            return this.allProperties;
        }

        /// <summary>
        /// Validate the given <paramref name="property"/> and adds it to the list of properties for this type
        /// </summary>
        /// <param name="property">property which needs to be added.</param>
        private void AddPropertyInternal(ResourceProperty property)
        {
            if (this.propertiesDeclaredOnThisType == null)
            {
                this.propertiesDeclaredOnThisType = new List<ResourceProperty>();
            }

            foreach (ResourceProperty resourceProperty in this.propertiesDeclaredOnThisType)
            {
                if (resourceProperty.Name == property.Name)
                {
                    throw new InvalidOperationException(Strings.ResourceType_PropertyWithSameNameAlreadyExists(resourceProperty.Name, this.FullName));
                }
            }

            if (property.IsOfKind(ResourcePropertyKind.Key))
            {
                if (this.baseType != null)
                {
                    throw new InvalidOperationException(Strings.ResourceType_NoKeysInDerivedTypes);
                }

                if (this.ResourceTypeKind != ResourceTypeKind.EntityType)
                {
                    throw new InvalidOperationException(Strings.ResourceType_KeyPropertiesOnlyOnEntityTypes);
                }

                Debug.Assert(property.TypeKind == ResourceTypeKind.Primitive, "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
                Debug.Assert(!property.IsOfKind(ResourcePropertyKind.ETag), "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
                Debug.Assert(property.IsOfKind(ResourcePropertyKind.Primitive), "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
            }

            if (property.IsOfKind(ResourcePropertyKind.ETag))
            {
                if (this.ResourceTypeKind != ResourceTypeKind.EntityType)
                {
                    throw new InvalidOperationException(Strings.ResourceType_ETagPropertiesOnlyOnEntityTypes);
                }
#if DEBUG

                Debug.Assert(property.TypeKind == ResourceTypeKind.Primitive, "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
                Debug.Assert(property.IsOfKind(ResourcePropertyKind.Primitive), "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
                Debug.Assert(!property.IsOfKind(ResourcePropertyKind.Key), "This check must have been done in ResourceProperty.ValidatePropertyParameters method");
#endif
            }

            this.propertiesDeclaredOnThisType.Add(property);
        }

        /// <summary>
        /// Gets the property info for the resource property declared on this type
        /// </summary>
        /// <param name="resourceProperty">Resource property instance to get the property info</param>
        /// <returns>Returns the propertyinfo object for the specified resource property.</returns>
        private PropertyInfo GetPropertyInfoDecaredOnThisType(ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            Debug.Assert(resourceProperty.CanReflectOnInstanceTypeProperty, "resourceProperty.CanReflectOnInstanceTypeProperty");

            if (this.propertyInfosDeclaredOnThisType == null)
            {
                this.propertyInfosDeclaredOnThisType = new Dictionary<ResourceProperty, PropertyInfo>(ReferenceEqualityComparer<ResourceProperty>.Instance);
            }

            PropertyInfo propertyInfo;
            if (!this.propertyInfosDeclaredOnThisType.TryGetValue(resourceProperty, out propertyInfo))
            {
                BindingFlags bindingFlags = WebUtil.PublicInstanceBindingFlags;
                propertyInfo = this.InstanceType.GetProperty(resourceProperty.Name, bindingFlags);
                if (propertyInfo == null)
                {
                    throw new DataServiceException(500, Strings.BadProvider_UnableToGetPropertyInfo(this.FullName, resourceProperty.Name));
                }

                this.propertyInfosDeclaredOnThisType.Add(resourceProperty, propertyInfo);
            }

            Debug.Assert(propertyInfo != null, "propertyInfo != null");
            return propertyInfo;
        }

        /// <summary>Given a resource type, builds the EntityPropertyMappingInfo for each of the dynamic entity property mapping attribute</summary>
        /// <param name="currentResourceType">Resouce type for which EntityPropertyMappingAttribute discovery is happening</param>
        private void BuildDynamicEpmInfo(ResourceType currentResourceType)
        {
            if (currentResourceType.BaseType != null)
            {
                this.BuildDynamicEpmInfo(currentResourceType.BaseType);
            }

            if (currentResourceType.HasEntityPropertyMappings)
            {
                foreach (EntityPropertyMappingAttribute epmAttr in currentResourceType.AllEpmInfo.ToList())
                {
                    this.BuildEpmInfo(epmAttr, currentResourceType, false);

                    if (this == currentResourceType)
                    {
                        if (!this.PropertyExistsInCurrentType(epmAttr))
                        {
                            this.InheritedEpmInfo.Add(epmAttr);
                            this.OwnEpmInfo.Remove(epmAttr);
                        }
                        else
                        {
                            Debug.Assert(this.OwnEpmInfo.SingleOrDefault(attr => Object.ReferenceEquals(epmAttr, attr)) != null, "Own epmInfo should already have the given instance");
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Does given property in the attribute exist in this type or one of it's base types
        /// </summary>
        /// <param name="epmAttr">Attribute which has PropertyName</param>
        /// <returns>true if property exists in current type, false otherwise</returns>
        private bool PropertyExistsInCurrentType(EntityPropertyMappingAttribute epmAttr)
        {
            int indexOfSeparator = epmAttr.SourcePath.IndexOf('/');
            String propertyToLookFor = indexOfSeparator == -1 ? epmAttr.SourcePath : epmAttr.SourcePath.Substring(0, indexOfSeparator);
            return this.PropertiesDeclaredOnThisType.Any(p => p.Name == propertyToLookFor);
        }

        /// <summary>
        /// Checks if the resource type is sealed. If not, it throws an InvalidOperationException.
        /// </summary>
        private void ThrowIfSealed()
        {
            if (this.isReadOnly)
            {
                throw new InvalidOperationException(Strings.ResourceType_Sealed(this.FullName));
            }
        }

        /// <summary>
        /// Calls the virtual LoadPropertiesDeclaredOnThisType method, if its not already called and then
        /// adds the properties returned by the method to the list of properties for this type.
        /// </summary>
        private void GetPropertiesDeclaredOnThisType()
        {
            // We just call the virtual LoadPropertiesDeclaredOnThisType method only once. If it hasn't been called yet,
            // then call the method and update the state to reflect that.
            if (!this.isLoadPropertiesMethodCalled)
            {
                foreach (ResourceProperty p in this.LoadPropertiesDeclaredOnThisType())
                {
                    this.AddPropertyInternal(p);

                    // if this type is already set to readonly, make sure that new properties returned by the virtual method
                    // are also set to readonly
                    if (this.IsReadOnly)
                    {
                        p.SetReadOnly();
                    }
                }

                this.isLoadPropertiesMethodCalled = true;
            }
        }

        /// <summary>
        /// This method is called only when the Properties property is called and the type is already set to read-only.
        /// This method validates all the properties w.r.t to the base type and calls SetReadOnly on all the properties.
        /// </summary>
        private void ValidateType()
        {
            Debug.Assert(this.isLoadPropertiesMethodCalled && this.IsReadOnly, "This method must be invoked only if LoadPropertiesDeclaredOnThisType has been called and the type is set to ReadOnly");

            if (this.BaseType != null)
            {
                // make sure that there are no properties with the same name. Properties with duplicate name within the type
                // is already checked in AddProperty method
                foreach (ResourceProperty rp in this.BaseType.Properties)
                {
                    if (this.propertiesDeclaredOnThisType.Where(p => p.Name == rp.Name).FirstOrDefault() != null)
                    {
                        throw new InvalidOperationException(Strings.ResourceType_PropertyWithSameNameAlreadyExists(rp.Name, this.FullName));
                    }
                }
            }
            else if (this.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                if (this.propertiesDeclaredOnThisType.Where(p => p.IsOfKind(ResourcePropertyKind.Key)).FirstOrDefault() == null)
                {
                    throw new InvalidOperationException(Strings.ResourceType_MissingKeyPropertiesForEntity(this.FullName));
                }
            }

            // set all the properties to readonly
            foreach (ResourceProperty p in this.propertiesDeclaredOnThisType)
            {
                p.SetReadOnly();

                // Note that we cache the propertyinfo objects for each CLR properties in the ResourceType class
                // rather than the ResourceProperty class because the same ResourceProperty instance can be added
                // to multiple ResourceType instances.
                if (p.CanReflectOnInstanceTypeProperty)
                {
                    this.GetPropertyInfoDecaredOnThisType(p);
                }
            }

            // Resolve EpmInfos now that everything in the type hierarchy is readonly
            try
            {
                if (this.EpmInfoInitialized == false)
                {
                    this.BuildDynamicEpmInfo(this);
                    this.EpmInfoInitialized = true;
                }
            }
            catch
            {
                // If an exception was thrown from this.BuildDynamicEpmInfo(this) method
                // EpmSourceTree and EpmTargetTree may be only half constructed and need to be reset.
                if (this.HasEntityPropertyMappings && !this.EpmInfoInitialized)
                {
                    this.epmInfo.Reset();
                }

                throw;
            }
        }

        #endregion Methods.

        #region EpmInfoPerResourceType

        /// <summary>Holder of Epm related data structure per resource type</summary>
        private sealed class EpmInfoPerResourceType
        {
            /// <summary>EpmSourceTree per <see cref="ResourceType"/></summary>
            private EpmSourceTree epmSourceTree;

            /// <summary>EpmTargetTree per <see cref="ResourceType"/></summary>
            private EpmTargetTree epmTargetTree;

            /// <summary>Inherited EpmInfo</summary>
            private List<EntityPropertyMappingAttribute> inheritedEpmInfo;

            /// <summary>Own EpmInfo</summary>
            private List<EntityPropertyMappingAttribute> ownEpmInfo;

            /// <summary>Property for obtaining EpmSourceTree for a type</summary>
            internal EpmSourceTree EpmSourceTree
            {
                get
                {
                    if (this.epmSourceTree == null)
                    {
                        this.epmSourceTree = new EpmSourceTree(this.EpmTargetTree);
                    }

                    return this.epmSourceTree;
                }
            }

            /// <summary>Property for obtaining EpmTargetTree for a type</summary>
            internal EpmTargetTree EpmTargetTree
            {
                get
                {
                    if (this.epmTargetTree == null)
                    {
                        this.epmTargetTree = new EpmTargetTree();
                    }

                    return this.epmTargetTree;
                }
            }

            /// <summary>Inherited EpmInfo</summary>
            internal List<EntityPropertyMappingAttribute> InheritedEpmInfo
            {
                get
                {
                    if (this.inheritedEpmInfo == null)
                    {
                        this.inheritedEpmInfo = new List<EntityPropertyMappingAttribute>();
                    }

                    return this.inheritedEpmInfo;
                }
            }

            /// <summary>Own EpmInfo</summary>
            internal List<EntityPropertyMappingAttribute> OwnEpmInfo
            {
                get
                {
                    if (this.ownEpmInfo == null)
                    {
                        this.ownEpmInfo = new List<EntityPropertyMappingAttribute>();
                    }

                    return this.ownEpmInfo;
                }
            }

            /// <summary>
            /// Removes all data created internally by ResourceType. This is needed when building epm 
            /// info fails since the trees may be left in undefined state (i.e. half constructed) and 
            /// if inherited EPM attributes exist duplicates will be added.
            /// </summary>
            internal void Reset()
            {
                this.epmTargetTree = null;
                this.epmSourceTree = null;
                this.inheritedEpmInfo = null;
            }
        }

        #endregion
    }
}
