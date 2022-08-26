//---------------------------------------------------------------------
// <copyright file="EpmContentDeSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Class used for deserializing EntityPropertyMappingAttribute content.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.Collections.Generic;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;
    using System.ServiceModel.Syndication;
#endregion

    /// <summary>DeSerializer for reading EPM content</summary>
    internal sealed class EpmContentDeSerializer
    {
        /// <summary><see cref="ResourceType"/> for which this serializer works</summary>
        private readonly ResourceType resourceType;

        /// <summary>Object for which this serializer works</summary>
        private readonly object element;

        /// <summary>
        /// Constructor creates contained serializers
        /// </summary>
        /// <param name="resourceType">Resource type being serialized</param>
        /// <param name="element">Instance of <paramref name="resourceType"/></param>
        internal EpmContentDeSerializer(ResourceType resourceType, object element)
        {
            Debug.Assert(resourceType.HasEntityPropertyMappings == true, "Must have entity property mappings to instantiate EpmContentDeSerializer");
            this.resourceType = resourceType;
            this.element = element;
            this.resourceType.EnsureEpmInfoAvailability();
        }

        /// <summary>Delegates to each of custom and syndication deserializers for serializing content</summary>
        /// <param name="item"><see cref="SyndicationItem"/> to deserialize</param>
        /// <param name="state">State of the deserializer</param>
        internal void DeSerialize(SyndicationItem item, EpmContentDeserializerState state)
        {
            if (this.resourceType.EpmTargetTree.SyndicationRoot.SubSegments.Count > 0)
            {
                new EpmSyndicationContentDeSerializer(item, state).DeSerialize(this.resourceType, this.element);
            }
            
            if (this.resourceType.EpmTargetTree.NonSyndicationRoot.SubSegments.Count > 0)
            {
                new EpmCustomContentDeSerializer(item, state).DeSerialize(this.resourceType, this.element);
            }
        }
        
        /// <summary>Representation of deserializer state</summary>
        internal sealed class EpmContentDeserializerState
        {
            /// <summary>Is current operation an update</summary>
            public bool IsUpdateOperation
            {
                get;
                set;
            }
            
            /// <summary>IUpdatable used for updating the object</summary>
            public UpdatableWrapper Updatable
            {
                get;
                set;
            }
            
            /// <summary>Service instance</summary>
            public IDataService Service
            {
                get;
                set;
            }

            /// <summary>Properties that have been applied</summary>
            public EpmAppliedPropertyInfo PropertiesApplied
            {
                get;
                set;
            }
        }

        /// <summary>Holder of information about properties applied during deserialization of an object</summary>
        internal sealed class EpmAppliedPropertyInfo
        {
            /// <summary>Properties already applied</summary>
            private List<EpmAppliedProperty> properties;
            
            /// <summary>Map from properties to their corresponding type names</summary>
            private List<EpmPropertyToTypeMappingElement> propertyToTypeNameMap;

            /// <summary>Propeties already applied</summary>
            private IList<EpmAppliedProperty> Properties
            {
                get
                {
                    if (this.properties == null)
                    {
                        this.properties = new List<EpmAppliedProperty>();
                    }
                    
                    return this.properties;
                }
            }

            /// <summary>Propeties to type name mappings</summary>
            private IList<EpmPropertyToTypeMappingElement> PropertyToTypeNameMap
            {
                get
                {
                    if (this.propertyToTypeNameMap == null)
                    {
                        this.propertyToTypeNameMap = new List<EpmPropertyToTypeMappingElement>();
                    }

                    return this.propertyToTypeNameMap;
                }
            }
            
            /// <summary>Adds the given property to the collection of applied ones</summary>
            /// <param name="propertyPath">Path of property</param>
            /// <param name="wholePathCovered">Does the path include all sub-properties</param>
            internal void AddAppliedProperty(String propertyPath, bool wholePathCovered)
            {
                this.Properties.Add(new EpmAppliedProperty { PropertyPath = propertyPath, ApplyRecursive = wholePathCovered });
            }

            /// <summary>Adds the given property to the collection of applied ones</summary>
            /// <param name="propertyPath">Path of property</param>
            /// <param name="typeName">Type of the property</param>
            internal void AddPropertyToTypeMapItem(String propertyPath, String typeName)
            {
                this.PropertyToTypeNameMap.Add(new EpmPropertyToTypeMappingElement { PropertyPath = propertyPath, TypeName = typeName });
            }

            /// <summary>Checks if the given path is already applied</summary>
            /// <param name="propertyPath">Given property path</param>
            /// <returns>true if the property has already been applied, false otherwise</returns>
            internal bool Lookup(String propertyPath)
            {
                return this.properties != null && this.Properties.Any(e => e.PropertyPath == propertyPath ||
                                                                     (e.ApplyRecursive == true && 
                                                                      e.PropertyPath.Length <= propertyPath.Length && e.PropertyPath == propertyPath.Substring(0, e.PropertyPath.Length)));                
            }

            /// <summary>Looksup the type given a property path</summary>
            /// <param name="propertyPath">Given property path</param>
            /// <returns>String containing mapped type name, null otherwise</returns>
            internal String MapPropertyToType(String propertyPath)
            {
                if (this.propertyToTypeNameMap != null)
                {
                    EpmPropertyToTypeMappingElement mapping = this.PropertyToTypeNameMap.FirstOrDefault(e => e.PropertyPath == propertyPath);
                    return mapping != null ? mapping.TypeName : null;
                }
                else
                {
                    return null;
                }
            }

            /// <summary>Property that is applied</summary>
            private sealed class EpmAppliedProperty
            {
                /// <summary>Path of property</summary>
                public String PropertyPath
                {
                    get;
                    set;
                }

                /// <summary>Is the property application path considered recursive</summary>
                public bool ApplyRecursive
                {
                    get;
                    set;
                }
            }
            
            /// <summary>Maps a property path with the type</summary>
            private sealed class EpmPropertyToTypeMappingElement
            {
                /// <summary>Path of property</summary>
                public String PropertyPath
                {
                    get;
                    set;
                }
                
                /// <summary>Type of the property</summary>
                public String TypeName
                {
                    get;
                    set;
                }
            }
        }        
    }
}
