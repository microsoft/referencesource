//---------------------------------------------------------------------
// <copyright file="BindingEntityInfo.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   BindingEntityInfo class
// </summary>
//
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
#region Namespaces
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
#endregion

    /// <summary>Type of property stored in BindingPropertyInfo.</summary>
    internal enum BindingPropertyKind
    {
        /// <summary>Property type is a complex type.</summary>
        BindingPropertyKindComplex,

        /// <summary>Property type is an entity type with keys.</summary>
        BindingPropertyKindEntity,

        /// <summary>Property is of an DataServiceCollection.</summary>
        BindingPropertyKindCollection
    }

    /// <summary>Cache of information about entity types and their observable properties</summary>
    internal class BindingEntityInfo
    {
        /// <summary>Object reference used as a 'False' flag.</summary>
        private static readonly object FalseObject = new object();

        /// <summary>Object reference used as a 'False' flag.</summary>
        private static readonly object TrueObject = new object();

        /// <summary>Lock on metadata caches.</summary>
        private static readonly ReaderWriterLockSlim metadataCacheLock = new ReaderWriterLockSlim();

        /// <summary>Types which are known not to be entity types.</summary>
        private static readonly HashSet<Type> knownNonEntityTypes = new HashSet<Type>(EqualityComparer<Type>.Default);

        /// <summary>Types which are known to be (or not) collection types.</summary>
        private static readonly Dictionary<Type, object> knownObservableCollectionTypes = new Dictionary<Type, object>(EqualityComparer<Type>.Default);

        /// <summary>Mapping between types and their corresponding entity information</summary>
        private static readonly Dictionary<Type, BindingEntityInfoPerType> bindingEntityInfos = new Dictionary<Type, BindingEntityInfoPerType>(EqualityComparer<Type>.Default);

        /// <summary>Obtain binding info corresponding to a given type</summary>
        /// <param name="entityType">Type for which to obtain information</param>
        /// <returns>Info about the <paramref name="entityType"/></returns>
        internal static IList<BindingPropertyInfo> GetObservableProperties(Type entityType)
        {
            return GetBindingEntityInfoFor(entityType).ObservableProperties;
        }

        /// <summary>Gets the ClientType corresponding to the given type</summary>
        /// <param name="entityType">Input type</param>
        /// <returns>Corresponding ClientType</returns>
        internal static ClientType GetClientType(Type entityType)
        {
            return GetBindingEntityInfoFor(entityType).ClientType;
        }

        /// <summary>
        /// Get the entity set name for the target entity object.
        /// </summary>
        /// <param name="target">An entity object.</param>
        /// <param name="targetEntitySet">The 'currently known' entity set name for the target object.</param>
        /// <returns>The entity set name for the target object.</returns>
        /// <remarks>
        /// Allow user code to provide the entity set name. If user code does not provide the entity set name, then 
        /// this method will get the entity set name from the value of the EntitySetAttribute.
        /// The 'currently known' entity set name for top level collections can be provided through OEC constructor
        /// </remarks>
        internal static string GetEntitySet(
            object target,
            string targetEntitySet)
        {
            Debug.Assert(target != null, "Argument 'target' cannot be null.");
            Debug.Assert(BindingEntityInfo.IsEntityType(target.GetType()), "Argument 'target' must be an entity type.");

            // Here's the rules in order of priority for resolving entity set name
            // 1. EntitySet name passed in the constructor or extension methods of DataServiceCollection
            // 2. EntitySet name specified in the EntitySet attribute by the code gen. {Remember this attribute is
            //    not generated in case of MEST)
            if (!String.IsNullOrEmpty(targetEntitySet))
            {
                return targetEntitySet;
            }
            else
            {
                // If there is not a 'currently known' entity set name to validate against, then there must be 
                // EntitySet attribute on the entity type
                return BindingEntityInfo.GetEntitySetAttribute(target.GetType());
            }
        }

        /// <summary>
        /// Determine if the specified type is an DataServiceCollection.
        /// </summary>
        /// <remarks>
        /// If there a generic class in the inheritance hierarchy of the type, that has a single
        /// entity type paramenter T, and is assignable to DataServiceCollection(Of T), then
        /// the type is an DataServiceCollection.
        /// </remarks>
        /// <param name="collectionType">An object type specifier.</param>
        /// <returns>True if the type is an DataServiceCollection; otherwise false.</returns>
        internal static bool IsDataServiceCollection(Type collectionType)
        {
            Debug.Assert(collectionType != null, "Argument 'collectionType' cannot be null.");

            metadataCacheLock.EnterReadLock();
            try
            {
                object resultAsObject;
                if (knownObservableCollectionTypes.TryGetValue(collectionType, out resultAsObject))
                {
                    return resultAsObject == TrueObject;
                }
            }
            finally
            {
                metadataCacheLock.ExitReadLock();
            }

            Type type = collectionType;
            bool result = false;

            while (type != null)
            {
                if (type.IsGenericType)
                {
                    // Is there a generic class in the inheritance hierarchy, that has a single
                    // entity type paramenter T, and is assignable to DataServiceCollection<T>
                    Type[] parms = type.GetGenericArguments();

                    if (parms != null && parms.Length == 1 && IsEntityType(parms[0]))
                    {
                        // if ObservableCollection is not available dataServiceCollection will be null
                        Type dataServiceCollection = WebUtil.GetDataServiceCollectionOfT(parms);
                        if (dataServiceCollection != null && dataServiceCollection.IsAssignableFrom(type))
                        {
                            result = true;
                            break;
                        }
                    }
                }

                type = type.BaseType;
            }

            metadataCacheLock.EnterWriteLock();
            try
            {
                if (!knownObservableCollectionTypes.ContainsKey(collectionType))
                {
                    knownObservableCollectionTypes[collectionType] = result ? TrueObject : FalseObject;
                }
            }
            finally
            {
                metadataCacheLock.ExitWriteLock();
            }

            return result;
        }

        /// <summary>
        /// Determine if the specified type is an entity type.
        /// </summary>
        /// <param name="type">An object type specifier.</param>
        /// <returns>True if the type is an entity type; otherwise false.</returns>
        internal static bool IsEntityType(Type type)
        {
            Debug.Assert(type != null, "Argument 'type' cannot be null.");

            metadataCacheLock.EnterReadLock();
            try
            {
                if (knownNonEntityTypes.Contains(type))
                {
                    return false;
                }
            }
            finally
            {
                metadataCacheLock.ExitReadLock();
            }

            try
            {
                if (BindingEntityInfo.IsDataServiceCollection(type))
                {
                    return false;
                }

                return ClientType.Create(type).IsEntityType;
            }
            catch (InvalidOperationException)
            {
                metadataCacheLock.EnterWriteLock();
                try
                {
                    if (!knownNonEntityTypes.Contains(type))
                    {
                        knownNonEntityTypes.Add(type);
                    }
                }
                finally
                {
                    metadataCacheLock.ExitWriteLock();
                }

                return false;
            }
        }

        /// <summary>
        /// Gets the value of a property and corresponding BindingPropertyInfo if property is being observed
        /// </summary>
        /// <param name="source">Source object whose property needs to be read</param>
        /// <param name="sourceProperty">Name of the source object property</param>
        /// <param name="bindingPropertyInfo">BindingPropertyInfo corresponding to <paramref name="sourceProperty"/></param>
        /// <returns>Value of the property</returns>
        internal static object GetPropertyValue(object source, string sourceProperty, out BindingPropertyInfo bindingPropertyInfo)
        {
            Type sourceType = source.GetType();

            bindingPropertyInfo = BindingEntityInfo.GetObservableProperties(sourceType)
                                                   .SingleOrDefault(x => x.PropertyInfo.PropertyName == sourceProperty);

            // bindingPropertyInfo is null for primitive properties.
            if (bindingPropertyInfo == null)
            {
                return BindingEntityInfo.GetClientType(sourceType)
                                        .GetProperty(sourceProperty, false)
                                        .GetValue(source);
            }
            else
            {
                return bindingPropertyInfo.PropertyInfo.GetValue(source);
            }
        }

        /// <summary>Obtain binding info corresponding to a given type</summary>
        /// <param name="entityType">Type for which to obtain information</param>
        /// <returns>Info about the <paramref name="entityType"/></returns>
        private static BindingEntityInfoPerType GetBindingEntityInfoFor(Type entityType)
        {
            BindingEntityInfoPerType bindingEntityInfo;

            metadataCacheLock.EnterReadLock();
            try
            {
                if (bindingEntityInfos.TryGetValue(entityType, out bindingEntityInfo))
                {
                    return bindingEntityInfo;
                }
            }
            finally
            {
                metadataCacheLock.ExitReadLock();
            }

            bindingEntityInfo = new BindingEntityInfoPerType();

            // Try to get the entity set name from the EntitySetAttribute attributes. In order to make the
            // inheritance work, we need to look at the attributes declared in the base types also.
            object[] attributes = entityType.GetCustomAttributes(typeof(EntitySetAttribute), true);

            // There must be exactly one (unambiguous) EntitySetAttribute attribute.
            bindingEntityInfo.EntitySet = (attributes != null && attributes.Length == 1) ? ((EntitySetAttribute)attributes[0]).EntitySet : null;
            bindingEntityInfo.ClientType = ClientType.Create(entityType);
            
            foreach (ClientType.ClientProperty p in bindingEntityInfo.ClientType.Properties)
            {
                BindingPropertyInfo bpi = null;
            
                Type propertyType = p.PropertyType;
                
                if (p.CollectionType != null)
                {
                    if (BindingEntityInfo.IsDataServiceCollection(propertyType))
                    {
                        bpi = new BindingPropertyInfo { PropertyKind = BindingPropertyKind.BindingPropertyKindCollection };
                    }
                }
                else
                if (BindingEntityInfo.IsEntityType(propertyType))
                {
                    bpi = new BindingPropertyInfo { PropertyKind = BindingPropertyKind.BindingPropertyKindEntity };
                }
                else
                if (BindingEntityInfo.CanBeComplexProperty(p))
                {
                    // Add complex types and nothing else.
                    bpi = new BindingPropertyInfo { PropertyKind = BindingPropertyKind.BindingPropertyKindComplex };
                }
                
                if (bpi != null)
                {
                    bpi.PropertyInfo = p;
                    
                    // For complex types only treat complex typed properties as observable, we are not going to observer entity typed or primitive properties.
                    if (bindingEntityInfo.ClientType.IsEntityType || bpi.PropertyKind == BindingPropertyKind.BindingPropertyKindComplex)
                    {
                        bindingEntityInfo.ObservableProperties.Add(bpi);
                    }
                }
            }

            metadataCacheLock.EnterWriteLock();
            try
            {
                if (!bindingEntityInfos.ContainsKey(entityType))
                {
                    bindingEntityInfos[entityType] = bindingEntityInfo;
                }
            }
            finally
            {
                metadataCacheLock.ExitWriteLock();
            }

            return bindingEntityInfo;
        }

        /// <summary>Checks whether a given property can be a complex property i.e. implements INotifyPropertyChanged.</summary>
        /// <param name="property">Input property.</param>
        /// <returns>true if the property is complex property, false otherwise.</returns>
        private static bool CanBeComplexProperty(ClientType.ClientProperty property)
        {
            Debug.Assert(property != null, "property != null");
            if (typeof(INotifyPropertyChanged).IsAssignableFrom(property.PropertyType))
            {
                Debug.Assert(!property.IsKnownType, "Known types do not implement INotifyPropertyChanged.");
                return true;
            }

            return false;
        }

        /// <summary>Gets entity set corresponding to a given type</summary>
        /// <param name="entityType">Intput type</param>
        /// <returns>Entity set name for the type</returns>
        private static string GetEntitySetAttribute(Type entityType)
        {
            return GetBindingEntityInfoFor(entityType).EntitySet;
        }

        /// <summary>Information about a property interesting for binding</summary>
        internal class BindingPropertyInfo
        {
            /// <summary>Property information</summary>
            public ClientType.ClientProperty PropertyInfo
            {
                get;
                set;
            }

            /// <summary>Kind of the property i.e. complex, entity or collection.</summary>
            public BindingPropertyKind PropertyKind
            {
                get;
                set;
            }
        }

        /// <summary>Holder of information about entity properties for a type</summary>
        private sealed class BindingEntityInfoPerType
        {
            /// <summary>Collection of properties interesting to the observer</summary>
            private List<BindingPropertyInfo> observableProperties;

            /// <summary>Constructor</summary>
            public BindingEntityInfoPerType()
            {
                this.observableProperties = new List<BindingPropertyInfo>();
            }

            /// <summary>Entity set of the entity</summary>
            public String EntitySet
            {
                get;
                set;
            }

            /// <summary>Corresponding ClientTyp</summary>
            public ClientType ClientType
            {
                get;
                set;
            }

            /// <summary>Collection of properties interesting to the observer</summary>
            public List<BindingPropertyInfo> ObservableProperties
            {
                get
                {
                    return this.observableProperties;
                }
            }
        }

#if ASTORIA_LIGHT
        /// <summary>Read-writer lock, implemented over a Monitor.</summary>
        private sealed class ReaderWriterLockSlim
        {
            /// <summary>Single object on which to lock.</summary>
            private object _lock = new object();

            /// <summary>Enters a reader lock. Writers will also be blocked.</summary>            
            internal void EnterReadLock()
            {
                Monitor.Enter(_lock);
            }

            /// <summary>Enters a writer lock. Readers will also be blocked.</summary>
            internal void EnterWriteLock()
            {
                Monitor.Enter(_lock);
            }

            /// <summary>Exits a reader lock.</summary>
            internal void ExitReadLock()
            {
                Monitor.Exit(_lock);
            }

            /// <summary>Exits a writer lock.</summary>
            internal void ExitWriteLock()
            {
                Monitor.Exit(_lock);
            }
        }
#endif
    }
}
