//---------------------------------------------------------------------
// <copyright file="EpmContentSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Class used for serializing EntityPropertyMappingAttribute content.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.Collections.Generic;
    using System.Data.Services.Internal;
    using System.Data.Services.Providers;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.Linq;
    using System.ServiceModel.Syndication;
#endregion

    /// <summary>
    /// Publically visible interface of the Content serializer, acts as container for both Custom and Syndication content serializers
    /// </summary>
    internal sealed class EpmContentSerializer : IDisposable
    {
        /// <summary><see cref="ResourceType"/> for which this serializer works</summary>
        private readonly ResourceType resourceType;

        /// <summary>
        /// Syndication specific content serializer
        /// </summary>
        private readonly EpmSyndicationContentSerializer epmSyndicationSerializer;

        /// <summary>
        /// Custom content serializer
        /// </summary>
        private readonly EpmCustomContentSerializer epmCustomSerializer;

        /// <summary>
        /// Target syndication item to which we add serialized content
        /// </summary>
        private SyndicationItem targetItem;

        /// <summary>Collection of null valued properties for this current serialization</summary>
        private EpmNullValuedPropertyTree nullValuedProperties;

        /// <summary>
        /// Constructor creates contained serializers
        /// </summary>
        /// <param name="resourceType">Resource type being serialized</param>
        /// <param name="value">Instance of <paramref name="resourceType"/></param>
        /// <param name="targetItem">SyndicationItem to which content will be added</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal EpmContentSerializer(ResourceType resourceType, Object value, SyndicationItem targetItem, DataServiceProviderWrapper provider)
        {
            this.resourceType = resourceType;
            this.targetItem = targetItem;
            this.nullValuedProperties = new EpmNullValuedPropertyTree(provider, value);

            if (this.NeedEpmSerialization)
            {
                this.epmSyndicationSerializer = new EpmSyndicationContentSerializer(this.resourceType.EpmTargetTree, value, targetItem, this.nullValuedProperties);
                this.epmCustomSerializer = new EpmCustomContentSerializer(this.resourceType.EpmTargetTree, value, targetItem, this.nullValuedProperties, provider);
            }
        }

        /// <summary>Does this type have any entity property mappings at all</summary>
        private bool NeedEpmSerialization
        {
            get
            {
                Debug.Assert(this.resourceType != null, "Must have a valid ResourceType");
                return this.resourceType.HasEntityPropertyMappings;
            }
        }

        /// <summary>
        /// Cleansup visitor specific state of the target tree
        /// </summary>
        public void Dispose()
        {
            if (this.NeedEpmSerialization)
            {
                Debug.Assert(this.epmSyndicationSerializer != null, "ResourceType with mapping implies a valid syndication content serializer");
                this.epmSyndicationSerializer.Dispose();
                Debug.Assert(this.epmCustomSerializer != null, "ResourceType with mapping implies a valid custom content serializer");
                this.epmCustomSerializer.Dispose();
            }
        }

        /// <summary>Delegates to each of custom and syndication serializers for serializing content</summary>
        /// <param name="content">Content in which to write null valued properties</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal void Serialize(DictionaryContent content, DataServiceProviderWrapper provider)
        {
            if (this.NeedEpmSerialization)
            {
                Debug.Assert(this.epmSyndicationSerializer != null, "ResourceType with mapping implies a valid syndication content serializer");
                this.epmSyndicationSerializer.Serialize(provider);
                Debug.Assert(this.epmCustomSerializer != null, "ResourceType with mapping implies a valid custom content serializer");
                this.epmCustomSerializer.Serialize(provider);

                this.nullValuedProperties.AddNullValuesToContent(content);
            }
            else
            {
                Debug.Assert(this.targetItem != null, "Must always have target content item");
                this.targetItem.Authors.Add(new SyndicationPerson(null, String.Empty, null));
            }
        }

        /// <summary>Tree representing null valued properties for an instance</summary>
        internal sealed class EpmNullValuedPropertyTree
        {
            /// <summary>Root of the tree</summary>
            private EpmNullValuedPropertyNode root;

            /// <summary>Provider wrapper instance</summary>
            private DataServiceProviderWrapper provider;

            /// <summary>Object whose properties we will read</summary>
            private object element;

            /// <summary>Constructor that creates the root of the tree</summary>
            /// <param name="provider">Provider wrapper instance</param>
            /// <param name="element">Object whose properties we will read</param>
            internal EpmNullValuedPropertyTree(DataServiceProviderWrapper provider, object element)
            {
                this.root = new EpmNullValuedPropertyNode { Name = null };
                this.provider = provider;
                this.element = element;
            }

            /// <summary>Adds a property to the null valued collection</summary>
            /// <param name="epmInfo">EpmInfo containing the property information such as path</param>
            internal void Add(EntityPropertyMappingInfo epmInfo)
            {
                Debug.Assert(epmInfo != null, "epmInfo != null");
                EpmNullValuedPropertyNode current = this.root;
                ResourceType currentType = epmInfo.DefiningType;
                object currentValue = this.element;

                // We are here because the epm path points to a null value. If the path is multiple level deep, we need to
                // know the first level the null value begins and we don't need to serialize deeper than that.
                // To serialize the complex properties correctly in the case they are not already in content, we also need
                // to find the type for each segment from root to the first segment that has the null property value.
                foreach (var segment in epmInfo.Attribute.SourcePath.Split('/'))
                {
                    EpmNullValuedPropertyNode child = current.Children.FirstOrDefault(c => c.Name == segment);
                    if (child != null)
                    {
                        // The current segment is already added to the tree, reuse it.
                        current = child;
                        currentValue = child.Element;
                        currentType = child.ResourceType;
                    }
                    else
                    {
                        EpmNullValuedPropertyNode newNode = new EpmNullValuedPropertyNode { Name = segment };
                        Debug.Assert(currentType != null, "currentType != null");
                        ResourceProperty property = currentType.TryResolvePropertyName(segment);

                        Debug.Assert(currentValue != null, "currentValue != null");
                        ProjectedWrapper projectedValue = currentValue as ProjectedWrapper;
                        if (projectedValue == null)
                        {
                            if (property != null)
                            {
                                currentValue = this.provider.GetPropertyValue(currentValue, property, currentType);
                                currentValue = currentValue == DBNull.Value ? null : currentValue;
                                currentType = property.ResourceType;
                            }
                            else
                            {
                                // Handle open property...
                                currentValue = this.provider.GetOpenPropertyValue(currentValue, segment);
                                currentValue = currentValue == DBNull.Value ? null : currentValue;
                                if (currentValue != null)
                                {
                                    // Get the type from the instance.
                                    currentType = this.provider.GetResourceType(currentValue);
                                }
                                else
                                {
                                    // We have a null open property at hand, we don't know its type.
                                    // Default the type to string so that we will omit the type name
                                    // and just write out null. i.e. <d:prop m:null='true'/>
                                    currentType = ResourceType.PrimitiveStringResourceType;
                                }
                            }
                        }
                        else
                        {
                            currentValue = projectedValue.GetProjectedPropertyValue(segment);
                            currentValue = currentValue == DBNull.Value ? null : currentValue;

                            if (property != null)
                            {
                                currentType = property.ResourceType;
                            }
                            else
                            {
                                // Handle open property...
                                if (currentValue == null)
                                {
                                    // We have a null open property at hand, we don't know its type.
                                    // Default the type to string so that we will omit the type name
                                    // and just write out null. i.e. <d:prop m:null='true'/>
                                    currentType = ResourceType.PrimitiveStringResourceType;
                                }
                                else
                                {
                                    projectedValue = currentValue as ProjectedWrapper;
                                    if (projectedValue != null)
                                    {
                                        // Get the type from the project wrapper.
                                        currentType = this.provider.TryResolveResourceType(projectedValue.ResourceTypeName);
                                    }
                                    else
                                    {
                                        // Get the type from the instance.
                                        currentType = this.provider.GetResourceType(currentValue);
                                    }
                                }
                            }
                        }

                        Debug.Assert(currentType != null, "currentType != null");
                        Debug.Assert(currentValue != DBNull.Value, "currentValue != DBNull.Value -- we have converted DBNull to null");
                        newNode.ResourceType = currentType;
                        newNode.Element = currentValue;
                        current.Children.Add(newNode);
                        current = newNode;
                    }

                    if (current.Element == null)
                    {
                        // If the current element is null, we don't need to go further since that is the obvious reason
                        // that the children properties are null.
                        break;
                    }
                }
            }

            /// <summary>Adds the null valued properties to the content section</summary>
            /// <param name="content">Content to which null properties are to be added</param>
            internal void AddNullValuesToContent(DictionaryContent content)
            {
                this.AddNullValuesToContent(this.root, content);
            }

            /// <summary>Adds the null valued properties to the content section of a syndication entry</summary>
            /// <param name="currentRoot">Current root node</param>
            /// <param name="currentContent">Current collection to which property is to be added</param>
            private void AddNullValuesToContent(EpmNullValuedPropertyNode currentRoot, DictionaryContent currentContent)
            {
                foreach (EpmNullValuedPropertyNode node in currentRoot.Children)
                {
                    bool found;
                    DictionaryContent c = currentContent.Lookup(node.Name, out found);

                    Debug.Assert(node.ResourceType != null, "node.ResourceType != null");
                    switch (node.ResourceType.ResourceTypeKind)
                    {
                        case ResourceTypeKind.ComplexType:
                            if (!found)
                            {
                                // If a complex property is not found in content, it is either not being projected
                                // or all of its properties are mapped and all of them have KeepInContent=false
                                Debug.Assert(c == null, "when look up not found, c should be null.");
                                if (node.Element != null)
                                {
                                    Debug.Assert(node.Children.Count > 0, "If the property represented by the current node is not null, there must be children nodes.");

                                    // The complex property is not null, but some of its descendant properties are null.
                                    // We need to serialize the type name of the complex property.
                                    c = new DictionaryContent();
                                    currentContent.Add(node.Name, node.ResourceType.FullName, c);
                                }
                                else
                                {
                                    Debug.Assert(node.Children.Count == 0, "If the property represented by the current node is not null, there must not be any children node.");

                                    // The complex property is null, we write out m:null='true'.
                                    currentContent.AddNull(node.ResourceType.FullName, node.Name);
                                }
                            }

                            if (c != null)
                            {
                                // Only add the children properties if the complex property is not null.
                                this.AddNullValuesToContent(node, c);
                            }

                            break;

                        case ResourceTypeKind.Primitive:
                            Debug.Assert(c == null, "DictionaryContent not expected for primitive properties.");
                            Debug.Assert(node.Element == null, "node.Element == null");
                            if (!found)
                            {
                                currentContent.AddNull(node.ResourceType.FullName, node.Name);
                            }

                            // if found, use the value in currentContent, we don't need to do anything here.
                            break;

                        case ResourceTypeKind.EntityType:
                            Debug.Assert(false, "We cannot map navigation properties with friendly feeds.");
                            break;
                    }
                }
            }

            /// <summary>
            /// Representation for a node in the <see cref="EpmNullValuedPropertyTree"/>
            /// </summary>
            private sealed class EpmNullValuedPropertyNode
            {
                /// <summary>Children of current node</summary>
                private List<EpmNullValuedPropertyNode> children;

                /// <summary>Name of the property</summary>
                internal String Name
                {
                    get;
                    set;
                }

                /// <summary>ResourceType corresponding to the node</summary>
                internal ResourceType ResourceType
                {
                    get;
                    set;
                }

                /// <summary>Object whose element we will read</summary>
                internal object Element
                {
                    get;
                    set;
                }

                /// <summary>Lazily creates the children collection and returns the collection</summary>
                internal ICollection<EpmNullValuedPropertyNode> Children
                {
                    get
                    {
                        if (this.children == null)
                        {
                            this.children = new List<EpmNullValuedPropertyNode>();
                        }

                        return this.children;
                    }
                }
            }
        }
    }
}
