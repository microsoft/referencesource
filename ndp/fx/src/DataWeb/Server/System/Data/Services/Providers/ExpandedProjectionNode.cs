//---------------------------------------------------------------------
// <copyright file="ExpandedProjectionNode.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Represents an expanded node in the tree of projections
//      for queries with $expand and/or $select.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    #endregion

    /// <summary>This class represents an expanded navigation property in the tree
    /// of projected properties. It is also used to represent the root of the projection tree.</summary>
    [DebuggerDisplay("ExpandedProjectionNode {PropertyName}")]
    internal class ExpandedProjectionNode : ProjectionNode
    {
        #region Private fields
        /// <summary>The resource set to which the expansion leads.</summary>
        /// <remarks>If this node represents expanded navigation property, this is the resource set
        /// to which the expanded navigation property points to.
        /// If this node is the root node of the projection tree, this is the resource set
        /// for the root of the query results.</remarks>
        private readonly ResourceSetWrapper resourceSetWrapper;

        /// <summary>Collection of information which describes the ordering for the results
        /// returned by this expanded property.</summary>
        /// <remarks>This can be null in which case no ordering is to be applied.</remarks>
        private readonly OrderingInfo orderingInfo;

        /// <summary>The filter expression to be applied to results returned by this expanded property.</summary>
        /// <remarks>This can be null in which case no filter is to be applied.</remarks>
        private readonly Expression filter;

        /// <summary>Number of results to skip for this node.</summary>
        /// <remarks>null value means that no skip should be applied.</remarks>
        private readonly int? skipCount;

        /// <summary>Maximum number of results to return for this node.</summary>
        /// <remarks>null value means that all results should be returned.</remarks>
        private readonly int? takeCount;

        /// <summary>Maximum number of results allowed for this node. Provider should use this only as a hint.
        /// It should return no less then maxResultsExpected + 1 results (assuming that number is available)
        /// so that the service can detect violation of the limit.</summary>
        /// <remarks>null value means that no limit will be applied and thus all results available should be returned.</remarks>
        private readonly int? maxResultsExpected;

        /// <summary>List of child nodes.</summary>
        private List<ProjectionNode> nodes;

        /// <summary>Internal field which is set to true once we have seen a projection including this expanded
        /// property. Otherwise set to false.</summary>
        /// <remarks>This field is used to eliminate expanded nodes which are not projected and thus there
        /// would be no point in expanding them.</remarks>
        private bool projectionFound;

        /// <summary>Flag which specifies if all child properties of this node should be projected.</summary>
        private bool projectAllImmediateProperties;

        /// <summary>Flag which specified is the entire expanded subtree of this node should be projected.</summary>
        private bool projectSubtree;
        #endregion

        #region Constructors
        /// <summary>Creates new instance of node representing expanded navigation property.</summary>
        /// <param name="propertyName">The name of the property to project and expand.</param>
        /// <param name="property">The <see cref="ResourceProperty"/> for this property. Can only be null for the root node.</param>
        /// <param name="resourceSetWrapper">The resource set to which the expansion leads.</param>
        /// <param name="orderingInfo">The ordering info for this node. null means no ordering to be applied.</param>
        /// <param name="filter">The filter for this node. null means no filter to be applied.</param>
        /// <param name="skipCount">Number of results to skip. null means no results to be skipped.</param>
        /// <param name="takeCount">Maximum number of results to return. null means return all available results.</param>
        /// <param name="maxResultsExpected">Maximum number of expected results. Hint that the provider should return
        /// at least maxResultsExpected + 1 results (if available).</param>
        internal ExpandedProjectionNode(
            string propertyName,
            ResourceProperty property,
            ResourceSetWrapper resourceSetWrapper,
            OrderingInfo orderingInfo,
            Expression filter,
            int? skipCount,
            int? takeCount,
            int? maxResultsExpected)
            : base(propertyName, property)
        {
            Debug.Assert(resourceSetWrapper != null, "resourceSetWrapper != null");
            Debug.Assert(property != null || propertyName.Length == 0, "We don't support open navigation properties.");

            this.resourceSetWrapper = resourceSetWrapper;
            this.orderingInfo = orderingInfo;
            this.filter = filter;
            this.skipCount = skipCount;
            this.takeCount = takeCount;
            this.maxResultsExpected = maxResultsExpected;
            this.nodes = new List<ProjectionNode>();
        }
        #endregion

        #region Public properties
        /// <summary>Collection of information which describes the ordering for the results
        /// returned by this expanded property.</summary>
        /// <remarks>This can be null in which case no ordering is to be applied.</remarks>
        public OrderingInfo OrderingInfo
        {
            get
            {
                return this.orderingInfo;
            }
        }

        /// <summary>The filter expression to be applied to results returned by this expanded property.</summary>
        /// <remarks>This can be null in which case no filter is to be applied.</remarks>
        public Expression Filter
        {
            get
            {
                return this.filter;
            }
        }

        /// <summary>Number of results to skip for this node.</summary>
        /// <remarks>null value means that no skip should be applied.</remarks>
        public int? SkipCount
        {
            get
            {
                return this.skipCount;
            }
        }

        /// <summary>Maximum number of results to return for this node.</summary>
        /// <remarks>null value means that all results should be returned.</remarks>
        public int? TakeCount
        {
            get
            {
                return this.takeCount;
            }
        }

        /// <summary>Maximum number of results allowed for this node. Provider should use this only as a hint.
        /// It should return no less then MaxResultsExpected + 1 results (assuming that number is available)
        /// so that the service can detect violation of the limit.</summary>
        /// <remarks>null value means that no limit will be applied and thus all results available should be returned.</remarks>
        public int? MaxResultsExpected
        {
            get
            {
                return this.maxResultsExpected;
            }
        }

        /// <summary>List of child nodes.</summary>
        public IEnumerable<ProjectionNode> Nodes
        {
            get
            {
                return this.nodes;
            }
        }

        /// <summary>Set to true if all properties of this node should be made part of the results.</summary>
        public bool ProjectAllProperties
        {
            get
            {
                return this.projectSubtree || this.ProjectAllImmediateProperties;
            }
        }
        #endregion

        #region Internal properties
        /// <summary>The resource set to which the expansion leads.</summary>
        /// <remarks>If this node represents expanded navigation property, this is the resource set
        /// to which the expanded navigation property points to.
        /// If this node is the root node of the projection tree, this is the resource set
        /// for the root of the query results.
        /// This property is for internal use by components of the WCF Data Services
        /// to avoid unnecessary lookups of the wrapper from the ResourceSet property.</remarks>
        internal ResourceSetWrapper ResourceSetWrapper
        {
            get
            {
                return this.resourceSetWrapper;
            }
        }

        /// <summary>The resource type in which all the entities expanded by this segment will be of.</summary>
        /// <remarks>This is usually the resource type of the <see cref="ResourceSetWrapper"/> for this node,
        /// but it can also be a derived type of that resource type.
        /// This can happen if navigation property points to a resource set but uses a derived type.
        /// It can also happen if service operation returns entities from a given resource set
        /// but it returns derived types.</remarks>
        internal virtual ResourceType ResourceType
        {
            get
            {
                Debug.Assert(this.Property != null, "Derived class should override this if property can be null.");
                return this.Property.ResourceType;
            }
        }

        /// <summary>Internal property which is set to true once we have seen a projection including this expanded
        /// property. Otherwise set to false.</summary>
        /// <remarks>This property is used to eliminate expanded nodes which are not projected and thus there
        /// would be no point in expanding them.</remarks>
        internal bool ProjectionFound
        {
            get
            {
                return this.projectionFound;
            }

            set
            {
                this.projectionFound = value;
            }
        }

        /// <summary>Flag which specifies if all child properties of this node should be projected.</summary>
        internal bool ProjectAllImmediateProperties
        {
            get
            {
                return this.projectAllImmediateProperties;
            }

            set
            {
                Debug.Assert(!value || this.projectionFound, "Marking node to include all child properties requires the node to be projected.");
                this.projectAllImmediateProperties = value;
            }
        }

        /// <summary>Whether this expanded node has a filter or a constraint on max results returns.</summary>
        internal bool HasFilterOrMaxResults
        {
            get 
            { 
                return this.Filter != null || this.MaxResultsExpected.HasValue; 
            }
        }

        #endregion

        #region Internal methods
        /// <summary>Find a child node of a given property.</summary>
        /// <param name="propertyName">The name of the property to find the child for.</param>
        /// <returns>The child node if there's one for the specified <paramref name="propertyName"/> or null
        /// if no such child was found.</returns>
        internal ProjectionNode FindNode(string propertyName)
        {
            Debug.Assert(propertyName != null, "propertyName != null");
            return this.nodes.FirstOrDefault(
                projectionNode => String.Equals(projectionNode.PropertyName, propertyName, StringComparison.Ordinal));
        }

        /// <summary>Adds a new child node to this node.</summary>
        /// <param name="node">The child node to add.</param>
        internal void AddNode(ProjectionNode node)
        {
            Debug.Assert(node != null, "node != null");
            Debug.Assert(this.FindNode(node.PropertyName) == null, "Trying to add a duplicate node.");
            this.nodes.Add(node);
        }

        /// <summary>Walks the subtree of this node and removes all nodes which were not marked projected.</summary>
        /// <remarks>Used to remove unnecessary expanded nodes.</remarks>
        internal void RemoveNonProjectedNodes()
        {
            for (int j = this.nodes.Count - 1; j >= 0; j--)
            {
                ExpandedProjectionNode expandedNode = this.nodes[j] as ExpandedProjectionNode;

                // Leave non-expanded properties there as they specify projections.
                if (expandedNode == null)
                {
                    continue;
                }

                // If we are to project entire subtree, leave all expanded nodes in as well
                // otherwise remove the expanded nodes which are not marked as projected.
                if (!this.projectSubtree && !expandedNode.ProjectionFound)
                {
                    // This removes the expandedNode from the tree (and all its children)
                    this.nodes.RemoveAt(j);
                }
                else
                {
                    expandedNode.RemoveNonProjectedNodes();
                }
            }
        }

        /// <summary>Removes duplicates from the tree caused by wildcards and sorts the projected properties.</summary>
        /// <remarks>
        /// Examples
        /// $select=Orders, Orders/ID           - get rid of the Orders/ID
        /// $select=Orders, Orders/*            - get rid of the Orders/*
        /// $select=Orders/*, Orders/ID         - get rid of the Orders/ID
        /// $select=Orders/*, Orders/OrderItems&amp;$expand=Orders - get rid of the Orders/OrderItems (it's redundant to *)
        /// $select=Orders/*, Orders/OrderItems&amp;$expand=Orders/OrderItems - leave as is, the Orders/OrderItems are expanded
        /// 
        /// The sorting order is the same as the order in which the properties are enumerated on the owning type.
        /// This is to preserve the same order as if no projections occured.
        /// </remarks>
        internal void ApplyWildcardsAndSort()
        {
            // If this segment was marked to include entire subtree
            // simply remove all children which are not expanded
            // and propagate the information to all expanded children.
            if (this.projectSubtree)
            {
                for (int j = this.nodes.Count - 1; j >= 0; j--)
                {
                    ExpandedProjectionNode expandedNode = this.nodes[j] as ExpandedProjectionNode;
                    if (expandedNode != null)
                    {
                        expandedNode.projectSubtree = true;
                        expandedNode.ApplyWildcardsAndSort();
                    }
                    else
                    {
                        this.nodes.RemoveAt(j);
                    }
                }

                this.projectAllImmediateProperties = false;
                return;
            }

            for (int j = this.nodes.Count - 1; j >= 0; j--)
            {
                ExpandedProjectionNode expandedNode = this.nodes[j] as ExpandedProjectionNode;

                // If this node was marked to include all immediate properties, 
                //   remove all children which are not expanded.
                //   That means they are either simple properties or nav. properties which
                //   are not going to be expanded anyway.
                if (this.ProjectAllImmediateProperties && expandedNode == null)
                {
                    this.nodes.RemoveAt(j);
                }
                else if (expandedNode != null)
                {
                    expandedNode.ApplyWildcardsAndSort();
                }
            }

            if (this.nodes.Count > 0)
            {
                // Sort the subsegments such that they have the same order as the properties
                //   on the owning resource type.
                ResourceType resourceType = this.ResourceType;

                List<ProjectionNode> existingNodes = this.nodes;
                this.nodes = new List<ProjectionNode>(existingNodes.Count);
                foreach (ResourceProperty property in resourceType.Properties)
                {
                    Debug.Assert(
                        existingNodes.Where(node => node.Property == property).Count() <= 1,
                        "Can't have more than one projection segment for a given property.");
                    ProjectionNode projectionNode = existingNodes.FirstOrDefault(
                        node => node.Property == property);
                    if (projectionNode != null)
                    {
                        this.nodes.Add(projectionNode);
                    }
                }

                // And then append any open properties sorted alphabetically
                // We sort these since we don't want client to be able to influence
                //   the server behavior unless abo----ely necessary.
                List<ProjectionNode> openPropertyProjectionNodes =
                    existingNodes.Where(node => node.Property == null).ToList();
                openPropertyProjectionNodes.Sort(new Comparison<ProjectionNode>((x, y) =>
                {
                    return String.Compare(x.PropertyName, y.PropertyName, StringComparison.Ordinal);
                }));
                this.nodes.AddRange(openPropertyProjectionNodes);
                Debug.Assert(this.nodes.Count == existingNodes.Count, "We didn't sort all the properties.");
            }
        }

        /// <summary>Marks the entire subtree as projected.</summary>
        /// <remarks>This is used when there were no projections specified in the query
        /// to mark the entire tree as projected.</remarks>
        internal void MarkSubtreeAsProjected()
        {
            this.projectSubtree = true;
            this.projectAllImmediateProperties = false;

            foreach (ProjectionNode node in this.nodes)
            {
                ExpandedProjectionNode expandedNode = node as ExpandedProjectionNode;
                if (expandedNode != null)
                {
                    expandedNode.MarkSubtreeAsProjected();
                }
            }
        }
        #endregion
    }
}
