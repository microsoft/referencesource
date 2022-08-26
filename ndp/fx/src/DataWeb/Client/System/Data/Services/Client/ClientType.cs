//---------------------------------------------------------------------
// <copyright file="ClientType.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// type resolver
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    /// <summary>
    /// wrapper around a clr type to
    ///     get/set properties
    ///     add items to collections
    ///     support open types
    /// </summary>
    [DebuggerDisplay("{ElementTypeName}")]
    internal sealed class ClientType
    {
        /// <summary>what is the clr full name using ToString for generic name expansion</summary>
        internal readonly string ElementTypeName;

        /// <summary>what clr type does this represent</summary>
        internal readonly Type ElementType;

        /// <summary>if true then EntityType else if !KnownType then ComplexType else PrimitiveType</summary>
        internal readonly bool IsEntityType;

        /// <summary>count of keys on entity type</summary>
        internal readonly int KeyCount;

        #region static fields

        /// <summary>appdomain cache discovered types with properties that we materialize</summary>
        private static readonly Dictionary<Type, ClientType> types = new Dictionary<Type, ClientType>(EqualityComparer<Type>.Default);

        /// <summary>cache &lt;T&gt; and wireName to mapped type</summary>
        private static readonly Dictionary<TypeName, Type> namedTypes = new Dictionary<TypeName, Type>(new TypeNameEqualityComparer());
        #endregion

#if ASTORIA_OPEN_OBJECT
        /// <summary>IDictionary&lt;string,object&gt; OpenProperites { get; }</summary>
        private readonly ClientProperty openProperties;
#endif
        /// <summary>properties</summary>
        private ArraySet<ClientProperty> properties;

        /// <summary>Property that holds data for ATOM-style media link entries</summary>
        private ClientProperty mediaDataMember;

        /// <summary>Set to true if the type is marked as ATOM-style media link entry</summary>
        private bool mediaLinkEntry;

        /// <summary>Souce Epm mappings</summary>
        private EpmSourceTree epmSourceTree;
        
        /// <summary>Target Epm mappings</summary>
        private EpmTargetTree epmTargetTree;

        /// <summary>
        /// discover and prepare properties for usage
        /// </summary>
        /// <param name="type">type being processed</param>
        /// <param name="typeName">parameter name</param>
        /// <param name="skipSettableCheck">Whether the skip the check for settable properties.</param>
        private ClientType(Type type, string typeName, bool skipSettableCheck)
        {
            Debug.Assert(null != type, "null type");
            Debug.Assert(!String.IsNullOrEmpty(typeName), "empty typeName");

            this.ElementTypeName = typeName;
            this.ElementType = Nullable.GetUnderlyingType(type) ?? type;
#if ASTORIA_OPEN_OBJECT
            string openObjectPropertyName = null;
#endif
            if (!ClientConvert.IsKnownType(this.ElementType))
            {
#if ASTORIA_OPEN_OBJECT
                #region OpenObject determined by walking type hierarchy and looking for [OpenObjectAttribute("PropertyName")]
                Type openObjectDeclared = this.ElementType;
                for (Type tmp = openObjectDeclared; (null != tmp) && (typeof(object) != tmp); tmp = tmp.BaseType)
                {
                    object[] attributes = openObjectDeclared.GetCustomAttributes(typeof(OpenObjectAttribute), false);
                    if (1 == attributes.Length)
                    {
                        if (null != openObjectPropertyName)
                        {
                            throw Error.InvalidOperation(Strings.Clienttype_MultipleOpenProperty(this.ElementTypeName));
                        }

                        openObjectPropertyName = ((OpenObjectAttribute)attributes[0]).OpenObjectPropertyName;
                        openObjectDeclared = tmp;
                    }
                }
                #endregion
#endif

                Type keyPropertyDeclaredType = null;
                bool isEntity = type.GetCustomAttributes(true).OfType<DataServiceEntityAttribute>().Any();
                DataServiceKeyAttribute dska = type.GetCustomAttributes(true).OfType<DataServiceKeyAttribute>().FirstOrDefault();
                foreach (PropertyInfo pinfo in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
                {
                    //// examples where class<PropertyType>

                    //// the normal examples
                    //// PropertyType Property { get; set }
                    //// Nullable<PropertyType> Property { get; set; }

                    //// if 'Property: struct' then we would be unable set the property during construction (and have them stick)
                    //// but when its a class, we can navigate if non-null and set the nested properties
                    //// PropertyType Property { get; } where PropertyType: class

                    //// we do support adding elements to collections
                    //// ICollection<PropertyType> { get; /*ignored set;*/ }

                    //// indexed properties are not suported because 
                    //// we don't have anything to use as the index
                    //// PropertyType Property[object x] { /*ignored get;*/ /*ignored set;*/ }

                    //// also ignored 
                    //// if PropertyType.IsPointer (like byte*)
                    //// if PropertyType.IsArray except for byte[] and char[]
                    //// if PropertyType == IntPtr or UIntPtr

                    Type ptype = pinfo.PropertyType; // class / interface / value
                    ptype = Nullable.GetUnderlyingType(ptype) ?? ptype;

                    if (ptype.IsPointer ||
                        (ptype.IsArray && (typeof(byte[]) != ptype) && typeof(char[]) != ptype) ||
                        (typeof(IntPtr) == ptype) ||
                        (typeof(UIntPtr) == ptype))
                    {
                        continue;
                    }

                    Debug.Assert(!ptype.ContainsGenericParameters, "remove when test case is found that encounters this");

                    if (pinfo.CanRead &&
                        (!ptype.IsValueType || pinfo.CanWrite) &&
                        !ptype.ContainsGenericParameters &&
                        (0 == pinfo.GetIndexParameters().Length))
                    {
                        #region IsKey?
                        bool keyProperty = dska != null ? dska.KeyNames.Contains(pinfo.Name) : false;

                        if (keyProperty)
                        {
                            if (null == keyPropertyDeclaredType)
                            {
                                keyPropertyDeclaredType = pinfo.DeclaringType;
                            }
                            else if (keyPropertyDeclaredType != pinfo.DeclaringType)
                            {
                                throw Error.InvalidOperation(Strings.ClientType_KeysOnDifferentDeclaredType(this.ElementTypeName));
                            }

                            if (!ClientConvert.IsKnownType(ptype))
                            {
                                throw Error.InvalidOperation(Strings.ClientType_KeysMustBeSimpleTypes(this.ElementTypeName));
                            }

                            this.KeyCount++;
                        }
                        #endregion

#if ASTORIA_OPEN_OBJECT
                        #region IsOpenObjectProperty?
                        bool openProperty = (openObjectPropertyName == pinfo.Name) &&
                                              typeof(IDictionary<string, object>).IsAssignableFrom(ptype);
                        Debug.Assert(keyProperty != openProperty || (!keyProperty && !openProperty), "key can't be open type");
                        #endregion

                        ClientProperty property = new ClientProperty(pinfo, ptype, keyProperty, openProperty);

                        if (!property.OpenObjectProperty)
#else
                        ClientProperty property = new ClientProperty(pinfo, ptype, keyProperty);
#endif
                        {
                            if (!this.properties.Add(property, ClientProperty.NameEquality))
                            {
                                // 2nd property with same name shadows another property
                                int shadow = this.IndexOfProperty(property.PropertyName);
                                if (!property.DeclaringType.IsAssignableFrom(this.properties[shadow].DeclaringType))
                                {   // the new property is on the most derived class
                                    this.properties.RemoveAt(shadow);
                                    this.properties.Add(property, null);
                                }
                            }
                        }
#if ASTORIA_OPEN_OBJECT
                        else
                        {
                            if (pinfo.DeclaringType == openObjectDeclared)
                            {
                                this.openProperties = property;
                            }
                        }
#endif
                    }
                }

                #region No KeyAttribute, discover key by name pattern { DeclaringType.Name+ID, ID }
                if (null == keyPropertyDeclaredType)
                {
                    ClientProperty key = null;
                    for (int i = this.properties.Count - 1; 0 <= i; --i)
                    {
                        string propertyName = this.properties[i].PropertyName;
                        if (propertyName.EndsWith("ID", StringComparison.Ordinal))
                        {
                            string declaringTypeName = this.properties[i].DeclaringType.Name;
                            if ((propertyName.Length == (declaringTypeName.Length + 2)) &&
                                propertyName.StartsWith(declaringTypeName, StringComparison.Ordinal))
                            {
                                // matched "DeclaringType.Name+ID" pattern
                                if ((null == keyPropertyDeclaredType) ||
                                    this.properties[i].DeclaringType.IsAssignableFrom(keyPropertyDeclaredType))
                                {
                                    keyPropertyDeclaredType = this.properties[i].DeclaringType;
                                    key = this.properties[i];
                                }
                            }
                            else if ((null == keyPropertyDeclaredType) && (2 == propertyName.Length))
                            {
                                // matched "ID" pattern
                                keyPropertyDeclaredType = this.properties[i].DeclaringType;
                                key = this.properties[i];
                            }
                        }
                    }

                    if (null != key)
                    {
                        Debug.Assert(0 == this.KeyCount, "shouldn't have a key yet");
                        key.KeyProperty = true;
                        this.KeyCount++;
                    }
                }
                else if (this.KeyCount != dska.KeyNames.Count)
                {
                    var m = (from string a in dska.KeyNames
                             where null == (from b in this.properties
                                            where b.PropertyName == a
                                            select b).FirstOrDefault()
                             select a).First<string>();
                    throw Error.InvalidOperation(Strings.ClientType_MissingProperty(this.ElementTypeName, m));
                }
                #endregion

                this.IsEntityType = (null != keyPropertyDeclaredType) || isEntity;
                Debug.Assert(this.KeyCount == this.Properties.Where(k => k.KeyProperty).Count(), "KeyCount mismatch");

                this.WireUpMimeTypeProperties();
                this.CheckMediaLinkEntry();

                if (!skipSettableCheck)
                {
#if ASTORIA_OPEN_OBJECT
                    if ((0 == this.properties.Count) && (null == this.openProperties))
#else
                    if (0 == this.properties.Count)
#endif
                    {   // implicit construction?
                        throw Error.InvalidOperation(Strings.ClientType_NoSettableFields(this.ElementTypeName));
                    }
                }
            }

            this.properties.TrimToSize();
            this.properties.Sort<string>(ClientProperty.GetPropertyName, String.CompareOrdinal);

#if ASTORIA_OPEN_OBJECT
            #region Validate OpenObjectAttribute was used
            if ((null != openObjectPropertyName) && (null == this.openProperties))
            {
                throw Error.InvalidOperation(Strings.ClientType_MissingOpenProperty(this.ElementTypeName, openObjectPropertyName));
            }

            Debug.Assert((null != openObjectPropertyName) == (null != this.openProperties), "OpenProperties mismatch");
            #endregion
#endif
            this.BuildEpmInfo(type);
        }

        /// <summary>Properties sorted by name</summary>
        internal ArraySet<ClientProperty> Properties
        {
            get { return this.properties; }
        }

        /// <summary>Property that holds data for ATOM-style media link entries</summary>
        internal ClientProperty MediaDataMember
        {
            get { return this.mediaDataMember; }
        }

        /// <summary>Returns true if the type is marked as ATOM-style media link entry</summary>
        internal bool IsMediaLinkEntry
        {
            get { return this.mediaLinkEntry; }
        }

        /// <summary>
        /// Source tree for <see cref="EntityPropertyMappingAttribute"/>s on this type
        /// </summary>
        internal EpmSourceTree EpmSourceTree
        {
            get
            {
                if (this.epmSourceTree == null)
                {
                    this.epmTargetTree = new EpmTargetTree();
                    this.epmSourceTree = new EpmSourceTree(this.epmTargetTree);
                }
                
                return this.epmSourceTree;
            }
        }

        /// <summary>
        /// Target tree for <see cref="EntityPropertyMappingAttribute"/>s on this type
        /// </summary>
        internal EpmTargetTree EpmTargetTree
        {
            get
            {
                Debug.Assert(this.epmTargetTree != null, "Must have valid target tree");
                return this.epmTargetTree;
            }
        }

        /// <summary>Are there any entity property mappings on this type</summary>
        internal bool HasEntityPropertyMappings
        {
            get
            {
                return this.epmSourceTree != null;
            }
        }

        /// <summary>The mappings for friendly feeds are V1 compatible or not</summary>
        internal bool EpmIsV1Compatible
        {
            get
            {
                return !this.HasEntityPropertyMappings || this.EpmTargetTree.IsV1Compatible;
            }
        }

        /// <summary>Whether a variable of <paramref name="type"/> can be assigned null.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if a variable of type <paramref name="type"/> can be assigned null; false otherwise.</returns>
        internal static bool CanAssignNull(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return !type.IsValueType || (type.IsGenericType && (type.GetGenericTypeDefinition() == typeof(Nullable<>)));
        }

        /// <summary>
        /// Is the type or element type (in the case of nullableOfT or IEnumOfT) a Entity Type?
        /// </summary>
        /// <param name="t">Type to examine</param>
        /// <returns>bool indicating whether or not entity type</returns>
        internal static bool CheckElementTypeIsEntity(Type t)
        {
            t = TypeSystem.GetElementType(t);
            t = Nullable.GetUnderlyingType(t) ?? t;
            return ClientType.Create(t, false).IsEntityType;
        }

        /// <summary>
        /// get a client type resolver
        /// </summary>
        /// <param name="type">type to wrap</param>
        /// <returns>client type</returns>
        internal static ClientType Create(Type type)
        {
            return Create(type, true /* expectModelType */);
        }

        /// <summary>
        /// get a client type resolver
        /// </summary>
        /// <param name="type">type to wrap</param>
        /// <param name="expectModelType">Whether the type is expected to be a model type.</param>
        /// <returns>client type</returns>
        internal static ClientType Create(Type type, bool expectModelType)
        {
            ClientType clientType;
            lock (ClientType.types)
            {
                ClientType.types.TryGetValue(type, out clientType);
            }

            if (null == clientType)
            {
                if (CommonUtil.IsUnsupportedType(type))
                {
                    throw new InvalidOperationException(Strings.ClientType_UnsupportedType(type));
                }

                bool skipSettableCheck = !expectModelType;
                clientType = new ClientType(type, type.ToString(), skipSettableCheck); // ToString expands generic type name where as FullName does not
                if (expectModelType)
                {
                    lock (ClientType.types)
                    {
                        ClientType existing;
                        if (ClientType.types.TryGetValue(type, out existing))
                        {
                            clientType = existing;
                        }
                        else
                        {
                            ClientType.types.Add(type, clientType);
                        }
                    }
                }
            }

            return clientType;
        }

#if !ASTORIA_LIGHT
        /// <summary>
        /// resolve the wireName/userType pair to a CLR type
        /// </summary>
        /// <param name="wireName">type name sent by server</param>
        /// <param name="userType">type passed by user or on propertyType from a class</param>
        /// <returns>mapped clr type</returns>
        internal static Type ResolveFromName(string wireName, Type userType)
#else
        /// <summary>
        /// resolve the wireName/userType pair to a CLR type
        /// </summary>
        /// <param name="wireName">type name sent by server</param>
        /// <param name="userType">type passed by user or on propertyType from a class</param>
        /// <param name="contextType">typeof context for strongly typed assembly</param>
        /// <returns>mapped clr type</returns>
        internal static Type ResolveFromName(string wireName, Type userType, Type contextType)
#endif
        {
            Type foundType;

            TypeName typename;
            typename.Type = userType;
            typename.Name = wireName;

            // search the "wirename"-userType key in type cache
            bool foundInCache;
            lock (ClientType.namedTypes)
            {
                foundInCache = ClientType.namedTypes.TryGetValue(typename, out foundType);
            }

            // at this point, if we have seen this type before, we either have the resolved type "foundType", 
            // or we have tried to resolve it before but did not success, in which case foundType will be null.
            // Either way we should return what's in the cache since the result is unlikely to change.
            // We only need to keep on searching if there isn't an entry in the cache.
            if (!foundInCache)
            {
                string name = wireName;
                int index = wireName.LastIndexOf('.');
                if ((0 <= index) && (index < wireName.Length - 1))
                {
                    name = wireName.Substring(index + 1);
                }

                if (userType.Name == name)
                {
                    foundType = userType;
                }
                else
                {
#if !ASTORIA_LIGHT
                    // searching only loaded assemblies, not referenced assemblies
                    foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
#else
                    foreach (Assembly assembly in new Assembly[] { userType.Assembly, contextType.Assembly }.Distinct())
#endif
                    {
                        Type found = assembly.GetType(wireName, false);
                        ResolveSubclass(name, userType, found, ref foundType);

                        if (null == found)
                        {
                            Type[] types = null;
                            try
                            {
                                types = assembly.GetTypes();
                            }
                            catch (ReflectionTypeLoadException)
                            {
                            }

                            if (null != types)
                            {
                                foreach (Type t in types)
                                {
                                    ResolveSubclass(name, userType, t, ref foundType);
                                }
                            }
                        }
                    }
                }

                // The above search can all fail and leave "foundType" to be null
                // we should cache this result too so we won't waste time searching again.
                lock (ClientType.namedTypes)
                {
                    ClientType.namedTypes[typename] = foundType;
                }
            }

            return foundType;
        }

        /// <summary>
        /// get concrete type that implements the genericTypeDefinitation
        /// </summary>
        /// <param name="propertyType">starting type</param>
        /// <param name="genericTypeDefinition">the generic type definition to find</param>
        /// <returns>concrete type that implementats the generic type</returns>
        internal static Type GetImplementationType(Type propertyType, Type genericTypeDefinition)
        {
            if (IsConstructedGeneric(propertyType, genericTypeDefinition))
            {   // propertyType is ICollection<T>
                return propertyType;
            }
            else
            {
                Type implementationType = null;
                foreach (Type interfaceType in propertyType.GetInterfaces())
                {
                    if (IsConstructedGeneric(interfaceType, genericTypeDefinition))
                    {
                        if (null == implementationType)
                        {   // found implmentation of ICollection<T>
                            implementationType = interfaceType;
                        }
                        else
                        {   // ICollection<int> and ICollection<int?>
                            throw Error.NotSupported(Strings.ClientType_MultipleImplementationNotSupported);
                        }
                    }
                }

                return implementationType;
            }
        }

        /// <summary>
        /// Gets the Add method to add items to a collection of the specified type.
        /// </summary>
        /// <param name="collectionType">Type for the collection.</param>
        /// <param name="type">The element type in the collection if found; null otherwise.</param>
        /// <returns>The method to invoke to add to a collection of the specified type.</returns>
        internal static MethodInfo GetAddToCollectionMethod(Type collectionType, out Type type)
        {
            return GetCollectionMethod(collectionType, typeof(ICollection<>), "Add", out type);
        }

        /// <summary>
        /// Gets the Remove method to remove items from a collection of the specified type.
        /// </summary>
        /// <param name="collectionType">Type for the collection.</param>
        /// <param name="type">The element type in the collection if found; null otherwise.</param>
        /// <returns>The method to invoke to remove from a collection of the specified type.</returns>
        internal static MethodInfo GetRemoveFromCollectionMethod(Type collectionType, out Type type)
        {
            return GetCollectionMethod(collectionType, typeof(ICollection<>), "Remove", out type);
        }

        /// <summary>
        /// get element type, resolves ICollection and Nullable
        /// </summary>
        /// <param name="propertyType">starting type</param>
        /// <param name="genericTypeDefinition">the generic type definition to find</param>
        /// <param name="methodName">the method to search for</param>
        /// <param name="type">the collection type if found</param>
        /// <returns>element types</returns>
        internal static MethodInfo GetCollectionMethod(Type propertyType, Type genericTypeDefinition, string methodName, out Type type)
        {
            Debug.Assert(null != propertyType, "null propertyType");
            Debug.Assert(null != genericTypeDefinition, "null genericTypeDefinition");
            Debug.Assert(genericTypeDefinition.IsGenericTypeDefinition, "!IsGenericTypeDefinition");

            type = null;

            Type implementationType = GetImplementationType(propertyType, genericTypeDefinition);
            if (null != implementationType)
            {
                Type[] genericArguments = implementationType.GetGenericArguments();
                MethodInfo methodInfo = implementationType.GetMethod(methodName);
                Debug.Assert(null != methodInfo, "should have found the method");

#if DEBUG
                Debug.Assert(null != genericArguments, "null genericArguments");
                ParameterInfo[] parameters = methodInfo.GetParameters();
                if (0 < parameters.Length)
                {
                    // following assert was disabled for Contains which returns bool
                    // Debug.Assert(typeof(void) == methodInfo.ReturnParameter.ParameterType, "method doesn't return void");

                    Debug.Assert(genericArguments.Length == parameters.Length, "genericArguments don't match parameters");
                    for (int i = 0; i < genericArguments.Length; ++i)
                    {
                        Debug.Assert(genericArguments[i] == parameters[i].ParameterType, "parameter doesn't match generic argument");
                    }
                }
#endif
                type = genericArguments[genericArguments.Length - 1];
                return methodInfo;
            }

            return null;
        }

        /// <summary>
        /// create object using default constructor
        /// </summary>
        /// <returns>instance of propertyType</returns>
        internal object CreateInstance()
        {
            return Activator.CreateInstance(this.ElementType);
        }

        /// <summary>
        /// get property wrapper for a property name, might be method around open types for otherwise unknown properties
        /// </summary>
        /// <param name="propertyName">property name</param>
        /// <param name="ignoreMissingProperties">are missing properties ignored</param>
        /// <returns>property wrapper</returns>
        /// <exception cref="InvalidOperationException">for unknown properties on closed types</exception>
        internal ClientProperty GetProperty(string propertyName, bool ignoreMissingProperties)
        {
            int index = this.IndexOfProperty(propertyName);
            if (0 <= index)
            {
                return this.properties[index];
            }
#if ASTORIA_OPEN_OBJECT
            else if (null != this.openProperties)
            {
                return this.openProperties;
            }
#endif
            else if (!ignoreMissingProperties)
            {
                throw Error.InvalidOperation(Strings.ClientType_MissingProperty(this.ElementTypeName, propertyName));
            }

            return null;
        }

        /// <summary>
        /// Checks whether the specified <paramref name="type"/> is a 
        /// closed constructed type of the generic type.
        /// </summary>
        /// <param name="type">Type to check.</param>
        /// <param name="genericTypeDefinition">Generic type for checkin.</param>
        /// <returns>true if <paramref name="type"/> is a constructed type of <paramref name="genericTypeDefinition"/>.</returns>
        /// <remarks>The check is an immediate check; no inheritance rules are applied.</remarks>
        private static bool IsConstructedGeneric(Type type, Type genericTypeDefinition)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(!type.ContainsGenericParameters, "remove when test case is found that encounters this");
            Debug.Assert(genericTypeDefinition != null, "genericTypeDefinition != null");

            return type.IsGenericType && (type.GetGenericTypeDefinition() == genericTypeDefinition) && !type.ContainsGenericParameters;
        }

        /// <summary>
        /// is the type a visible subclass with correct name
        /// </summary>
        /// <param name="wireClassName">type name from server</param>
        /// <param name="userType">the type from user for materialization or property type</param>
        /// <param name="type">type being tested</param>
        /// <param name="existing">the previously discovered matching type</param>
        /// <exception cref="InvalidOperationException">if the mapping is ambiguous</exception>
        private static void ResolveSubclass(string wireClassName, Type userType, Type type, ref Type existing)
        {
            if ((null != type) && type.IsVisible && (wireClassName == type.Name) && userType.IsAssignableFrom(type))
            {
                if (null != existing)
                {
                    throw Error.InvalidOperation(Strings.ClientType_Ambiguous(wireClassName, userType));
                }

                existing = type;
            }
        }

        /// <summary>
        /// By going over EntityPropertyMappingInfoAttribute(s) defined on <paramref name="type"/>
        /// builds the corresponding EntityPropertyMappingInfo
        /// </summary>
        /// <param name="type">Type being looked at</param>
        private void BuildEpmInfo(Type type)
        {
            if (type.BaseType != null && type.BaseType != typeof(object))
            {
                this.BuildEpmInfo(type.BaseType);
            }

            foreach (EntityPropertyMappingAttribute epmAttr in type.GetCustomAttributes(typeof(EntityPropertyMappingAttribute), false))
            {
                this.BuildEpmInfo(epmAttr, type);
            }
        }

        /// <summary>
        /// Builds the EntityPropertyMappingInfo corresponding to an EntityPropertyMappingAttribute, also builds the delegate to
        /// be invoked in order to retrieve the property provided in the <paramref name="epmAttr"/>
        /// </summary>
        /// <param name="epmAttr">Source EntityPropertyMappingAttribute</param>
        /// <param name="definingType">ResourceType on which to look for the property</param>
        private void BuildEpmInfo(EntityPropertyMappingAttribute epmAttr, Type definingType)
        {
            this.EpmSourceTree.Add(new EntityPropertyMappingInfo(epmAttr, definingType, this));
        }

        /// <summary>get the index of a property</summary>
        /// <param name="propertyName">propertyName</param>
        /// <returns>index else -1</returns>
        private int IndexOfProperty(string propertyName)
        {
            return this.properties.IndexOf(propertyName, ClientProperty.GetPropertyName, String.Equals);
        }

        /// <summary>
        /// Find properties with dynamic MIME type related properties and 
        /// set the references from each ClientProperty to its related MIME type property
        /// </summary>
        private void WireUpMimeTypeProperties()
        {
            MimeTypePropertyAttribute attribute = (MimeTypePropertyAttribute)this.ElementType.GetCustomAttributes(typeof(MimeTypePropertyAttribute), true).SingleOrDefault();
            if (null != attribute)
            {
                int dataIndex, mimeTypeIndex;
                if ((0 > (dataIndex = this.IndexOfProperty(attribute.DataPropertyName))) ||
                    (0 > (mimeTypeIndex = this.IndexOfProperty(attribute.MimeTypePropertyName))))
                {
                    throw Error.InvalidOperation(Strings.ClientType_MissingMimeTypeProperty(attribute.DataPropertyName, attribute.MimeTypePropertyName));
                }

                Debug.Assert(0 <= dataIndex, "missing data property");
                Debug.Assert(0 <= mimeTypeIndex, "missing mime type property");
                this.Properties[dataIndex].MimeTypeProperty = this.Properties[mimeTypeIndex];
            }
        }

        /// <summary>
        /// Check if this type represents an ATOM-style media link entry and
        /// if so mark the ClientType as such
        /// </summary>
        private void CheckMediaLinkEntry()
        {
            object[] attributes = this.ElementType.GetCustomAttributes(typeof(MediaEntryAttribute), true);
            if (attributes != null && attributes.Length > 0)
            {
                Debug.Assert(attributes.Length == 1, "The AttributeUsage in the attribute definition should be preventing more than 1 per property");

                MediaEntryAttribute mediaEntryAttribute = (MediaEntryAttribute)attributes[0];
                this.mediaLinkEntry = true;

                int index = this.IndexOfProperty(mediaEntryAttribute.MediaMemberName);
                if (index < 0)
                {
                    throw Error.InvalidOperation(Strings.ClientType_MissingMediaEntryProperty(
                        mediaEntryAttribute.MediaMemberName));
                }

                this.mediaDataMember = this.properties[index];
            }

            attributes = this.ElementType.GetCustomAttributes(typeof(HasStreamAttribute), true);
            if (attributes != null && attributes.Length > 0)
            {
                Debug.Assert(attributes.Length == 1, "The AttributeUsage in the attribute definition should be preventing more than 1 per property");
                this.mediaLinkEntry = true;
            }
        }

        /// <summary>type + wireName combination</summary>
        private struct TypeName
        {
            /// <summary>type</summary>
            internal Type Type;

            /// <summary>type name from server</summary>
            internal string Name;
        }

        /// <summary>
        /// wrapper around property methods
        /// </summary>
        [DebuggerDisplay("{PropertyName}")]
        internal sealed class ClientProperty
        {
            /// <summary>property name for debugging</summary>
            internal readonly string PropertyName;

            /// <summary>Exact property type; possibly nullable but not necessarily so.</summary>
            internal readonly Type NullablePropertyType;

            /// <summary>type of the property</summary>
            internal readonly Type PropertyType;

            /// <summary>what is the nested collection element</summary>
            internal readonly Type CollectionType;

            /// <summary>
            /// Is this a known primitive/reference type or an entity/complex/collection type?
            /// </summary>
            internal readonly bool IsKnownType;

#if ASTORIA_OPEN_OBJECT
            /// <summary>IDictionary&lt;string,object&gt; OpenProperites { get; }</summary>
            internal readonly bool OpenObjectProperty;
#endif

            /// <summary>property getter</summary>
            private readonly MethodInfo propertyGetter;

            /// <summary>property setter</summary>
            private readonly MethodInfo propertySetter;

            /// <summary>"set_Item" method supporting IDictionary properties</summary>
            private readonly MethodInfo setMethod;

            /// <summary>"Add" method supporting ICollection&lt;&gt; properties</summary>
            private readonly MethodInfo addMethod;

            /// <summary>"Remove" method supporting ICollection&lt;&gt; properties</summary>
            private readonly MethodInfo removeMethod;

            /// <summary>"Contains" method support ICollection&lt;&gt; properties</summary>
            private readonly MethodInfo containsMethod;

            /// <summary>IsKeyProperty?</summary>
            private bool keyProperty;

            /// <summary>The other property in this type that holds the MIME type for this one</summary>
            private ClientProperty mimeTypeProperty;

#if ASTORIA_OPEN_OBJECT
            /// <summary>
            /// constructor
            /// </summary>
            /// <param name="property">property</param>
            /// <param name="propertyType">propertyType</param>
            /// <param name="keyProperty">keyProperty</param>
            /// <param name="openObjectProperty">openObjectProperty</param>
            internal ClientProperty(PropertyInfo property, Type propertyType, bool keyProperty, bool openObjectProperty)
#else
            /// <summary>
            /// constructor
            /// </summary>
            /// <param name="property">property</param>
            /// <param name="propertyType">propertyType</param>
            /// <param name="keyProperty">Whether this property is part of the key for its declaring type.</param>
            internal ClientProperty(PropertyInfo property, Type propertyType, bool keyProperty)
#endif
            {
                Debug.Assert(null != property, "null property");
                Debug.Assert(null != propertyType, "null propertyType");
                Debug.Assert(null == Nullable.GetUnderlyingType(propertyType), "should already have been denullified");

                this.PropertyName = property.Name;
                this.NullablePropertyType = property.PropertyType;
                this.PropertyType = propertyType;
                this.propertyGetter = property.GetGetMethod();
                this.propertySetter = property.GetSetMethod();
                this.keyProperty = keyProperty;
#if ASTORIA_OPEN_OBJECT
                this.OpenObjectProperty = openObjectProperty;
#endif

                this.IsKnownType = ClientConvert.IsKnownType(propertyType);
                if (!this.IsKnownType)
                {
                    this.setMethod = GetCollectionMethod(this.PropertyType, typeof(IDictionary<,>), "set_Item", out this.CollectionType);
                    if (null == this.setMethod)
                    {
                        this.containsMethod = GetCollectionMethod(this.PropertyType, typeof(ICollection<>), "Contains", out this.CollectionType);
                        this.addMethod = GetAddToCollectionMethod(this.PropertyType, out this.CollectionType);
                        this.removeMethod = GetRemoveFromCollectionMethod(this.PropertyType, out this.CollectionType);
                    }
                }

                Debug.Assert(!this.keyProperty || this.IsKnownType, "can't have an random type as key");
            }

            /// <summary>what type was this property declared on?</summary>
            internal Type DeclaringType
            {
                get { return this.propertyGetter.DeclaringType; }
            }

            /// <summary>Does this property particpate in the primary key?</summary>
            internal bool KeyProperty
            {
                get { return this.keyProperty; }
                set { this.keyProperty = value; }
            }

            /// <summary>The other property in this type that holds the MIME type for this one</summary>
            internal ClientProperty MimeTypeProperty
            {
                get { return this.mimeTypeProperty; }
                set { this.mimeTypeProperty = value; }
            }

            /// <summary>get KeyProperty</summary>
            /// <param name="x">x</param>
            /// <returns>KeyProperty</returns>
            internal static bool GetKeyProperty(ClientProperty x)
            {
                return x.KeyProperty;
            }

            /// <summary>get property name</summary>
            /// <param name="x">x</param>
            /// <returns>PropertyName</returns>
            internal static string GetPropertyName(ClientProperty x)
            {
                return x.PropertyName;
            }

            /// <summary>compare name equality</summary>
            /// <param name="x">x</param>
            /// <param name="y">y</param>
            /// <returns>true if the property names are equal; false otherwise.</returns>
            internal static bool NameEquality(ClientProperty x, ClientProperty y)
            {
                return String.Equals(x.PropertyName, y.PropertyName);
            }

            /// <summary>
            /// get property value from an object
            /// </summary>
            /// <param name="instance">object to get the property value from</param>
            /// <returns>property value</returns>
            internal object GetValue(object instance)
            {
                Debug.Assert(null != instance, "null instance");
                Debug.Assert(null != this.propertyGetter, "null propertyGetter");
                return this.propertyGetter.Invoke(instance, null);
            }

            /// <summary>
            /// remove a item from collection
            /// </summary>
            /// <param name="instance">collection</param>
            /// <param name="value">item to remove</param>
            internal void RemoveValue(object instance, object value)
            {
                Debug.Assert(null != instance, "null instance");
                Debug.Assert(null != this.removeMethod, "missing removeMethod");

                Debug.Assert(this.PropertyType.IsAssignableFrom(instance.GetType()), "unexpected collection instance");
                Debug.Assert((null == value) || this.CollectionType.IsAssignableFrom(value.GetType()), "unexpected collection value to add");
                this.removeMethod.Invoke(instance, new object[] { value });
            }

#if ASTORIA_OPEN_OBJECT
            /// <summary>
            /// set property value on an object
            /// </summary>
            /// <param name="instance">object to set the property value on</param>
            /// <param name="value">property value</param>
            /// <param name="propertyName">used for open type</param>
            /// <param name="allowAdd">allow add to a collection if available, else allow setting collection property</param>
            /// <param name="openProperties">cached OpenProperties dictionary</param>
            internal void SetValue(object instance, object value, string propertyName, ref object openProperties, bool allowAdd)
#else
            /// <summary>
            /// set property value on an object
            /// </summary>
            /// <param name="instance">object to set the property value on</param>
            /// <param name="value">property value</param>
            /// <param name="propertyName">used for open type</param>
            /// <param name="allowAdd">allow add to a collection if available, else allow setting collection property</param>
            internal void SetValue(object instance, object value, string propertyName, bool allowAdd)
#endif
            {
                Debug.Assert(null != instance, "null instance");
                if (null != this.setMethod)
                {
#if ASTORIA_OPEN_OBJECT
                    if (this.OpenObjectProperty)
                    {
                        if (null == openProperties)
                        {
                            if (null == (openProperties = this.propertyGetter.Invoke(instance, null)))
                            {
                                throw Error.NotSupported(Strings.ClientType_NullOpenProperties(this.PropertyName));
                            }
                        }

                        ((IDictionary<string, object>)openProperties)[propertyName] = value;
                    }
                    else
#endif
                    {
                        Debug.Assert(this.PropertyType.IsAssignableFrom(instance.GetType()), "unexpected dictionary instance");
                        Debug.Assert((null == value) || this.CollectionType.IsAssignableFrom(value.GetType()), "unexpected dictionary value to set");

                        // ((IDictionary<string, CollectionType>)instance)[propertyName] = (CollectionType)value;
                        this.setMethod.Invoke(instance, new object[] { propertyName, value });
                    }
                }
                else if (allowAdd && (null != this.addMethod))
                {
                    Debug.Assert(this.PropertyType.IsAssignableFrom(instance.GetType()), "unexpected collection instance");
                    Debug.Assert((null == value) || this.CollectionType.IsAssignableFrom(value.GetType()), "unexpected collection value to add");

                    // ((ICollection<CollectionType>)instance).Add((CollectionType)value);
                    if (!(bool)this.containsMethod.Invoke(instance, new object[] { value }))
                    {
                        this.addMethod.Invoke(instance, new object[] { value });
                    }
                }
                else if (null != this.propertySetter)
                {
                    Debug.Assert((null == value) || this.PropertyType.IsAssignableFrom(value.GetType()), "unexpected property value to set");

                    // ((ElementType)instance).PropertyName = (PropertyType)value;
                    this.propertySetter.Invoke(instance, new object[] { value });
                }
                else
                {
                    throw Error.InvalidOperation(Strings.ClientType_MissingProperty(value.GetType().ToString(), propertyName));
                }
            }
        }

        /// <summary>equality comparer for TypeName</summary>
        private sealed class TypeNameEqualityComparer : IEqualityComparer<TypeName>
        {
            /// <summary>equality comparer for TypeName</summary>
            /// <param name="x">left type</param>
            /// <param name="y">right type</param>
            /// <returns>true if x and y are equal</returns>
            public bool Equals(TypeName x, TypeName y)
            {
                return (x.Type == y.Type && x.Name == y.Name);
            }

            /// <summary>compute hashcode for TypeName</summary>
            /// <param name="obj">object to compute hashcode for</param>
            /// <returns>computed hashcode</returns>
            public int GetHashCode(TypeName obj)
            {
                return obj.Type.GetHashCode() ^ obj.Name.GetHashCode();
            }
        }
    }
}
