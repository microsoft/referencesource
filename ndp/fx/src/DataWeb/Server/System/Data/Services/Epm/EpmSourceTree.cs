//---------------------------------------------------------------------
// <copyright file="EpmSourceTree.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Tree for managing SourceNames on EntityPropertyMappingAttributes
// for a ResourceType.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
#if ASTORIA_CLIENT
    using System.Data.Services.Client;
#else
    using System.Data.Services;
#endif

    /// <summary>
    /// Tree representing the sourceName properties in all the EntityPropertyMappingAttributes for a resource type
    /// </summary>
    internal sealed class EpmSourceTree
    {
        #region Fields

        /// <summary>Root of the tree</summary>
        private readonly EpmSourcePathSegment root;
        
        /// <summary><see cref="EpmTargetTree"/> corresponding to this tree</summary>
        private readonly EpmTargetTree epmTargetTree;

        #endregion

        /// <summary>Default constructor creates a null root</summary>
        /// <param name="epmTargetTree">Target xml tree</param>
        internal EpmSourceTree(EpmTargetTree epmTargetTree)
        {
            this.root = new EpmSourcePathSegment("");
            this.epmTargetTree = epmTargetTree;
        }

        #region Properties

        /// <summary>
        /// Root of the tree
        /// </summary>
        internal EpmSourcePathSegment Root
        {
            get
            {
                return this.root;
            }
        }

        #endregion

        /// <summary>
        /// Adds a path to the source and target tree which is obtained by looking at the EntityPropertyMappingAttribute in the <paramref name="epmInfo"/>
        /// </summary>
        /// <param name="epmInfo">EnitityPropertyMappingInfo holding the source path</param>
        internal void Add(EntityPropertyMappingInfo epmInfo)
        {
            String sourceName = epmInfo.Attribute.SourcePath;
            EpmSourcePathSegment currentProperty = this.Root;
            IList<EpmSourcePathSegment> activeSubProperties = currentProperty.SubProperties;
            EpmSourcePathSegment foundProperty = null;

            Debug.Assert(!String.IsNullOrEmpty(sourceName), "Must have been validated during EntityPropertyMappingAttribute construction");
            foreach (String propertyName in sourceName.Split('/'))
            {
                if (propertyName.Length == 0)
                {
                    throw new InvalidOperationException(Strings.EpmSourceTree_InvalidSourcePath(epmInfo.DefiningType.Name, sourceName));
                }

                foundProperty = activeSubProperties.SingleOrDefault(e => e.PropertyName == propertyName);
                if (foundProperty != null)
                {
                    currentProperty = foundProperty;
                }
                else
                {
                    currentProperty = new EpmSourcePathSegment(propertyName);
                    activeSubProperties.Add(currentProperty);
                }

                activeSubProperties = currentProperty.SubProperties;
            }

            // Two EpmAttributes with same PropertyName in the same ResourceType, this could be a result of inheritance
            if (foundProperty != null)
            {
                Debug.Assert(Object.ReferenceEquals(foundProperty, currentProperty), "currentProperty variable should have been updated already to foundProperty");

                // Check for duplicates on the same entity type
#if !ASTORIA_CLIENT
                Debug.Assert(foundProperty.SubProperties.Count == 0, "If non-leaf, it means we allowed complex type to be a leaf node");
                if (foundProperty.EpmInfo.DefiningType == epmInfo.DefiningType)
                {
                    throw new InvalidOperationException(Strings.EpmSourceTree_DuplicateEpmAttrsWithSameSourceName(epmInfo.Attribute.SourcePath, epmInfo.DefiningType.Name));
                }
#else                
                if (foundProperty.EpmInfo.DefiningType.Name == epmInfo.DefiningType.Name)
                {
                    throw new InvalidOperationException(Strings.EpmSourceTree_DuplicateEpmAttrsWithSameSourceName(epmInfo.Attribute.SourcePath, epmInfo.DefiningType.Name));
                }
#endif

                // In case of inheritance, we need to remove the node from target tree which was mapped to base type property
                this.epmTargetTree.Remove(foundProperty.EpmInfo);
            }

            currentProperty.EpmInfo = epmInfo;
            this.epmTargetTree.Add(epmInfo);
        }
    }
}
