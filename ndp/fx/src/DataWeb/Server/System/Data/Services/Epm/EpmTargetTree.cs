//---------------------------------------------------------------------
// <copyright file="EpmTargetTree.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Tree for managing TargetNames on EntityPropertyMappingAttributes
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
    /// Tree representing the targetName properties in all the EntityPropertyMappingAttributes for a resource type
    /// </summary>
    internal sealed class EpmTargetTree
    {
        /// <summary>Number of properties that have KeepInContent false</summary>
        private int countOfNonContentProperties;

        /// <summary>Initializes the sub-trees for syndication and non-syndication content</summary>
        internal EpmTargetTree()
        {
            this.SyndicationRoot = new EpmTargetPathSegment();
            this.NonSyndicationRoot = new EpmTargetPathSegment();
        }

        /// <summary>Root of the sub-tree for syndication content</summary>
        internal EpmTargetPathSegment SyndicationRoot
        {
            get; 
            private set;
        }

        /// <summary>Root of the sub-tree for custom content</summary>
        internal EpmTargetPathSegment NonSyndicationRoot
        {
            get;
            private set;
        }

        /// <summary>
        /// Does the target tree serialize data in V1 compatible manner
        /// </summary>
        internal bool IsV1Compatible
        {
            get
            {
                return this.countOfNonContentProperties == 0;
            }
        }

        /// <summary>
        /// Adds a path to the tree which is obtained by looking at the EntityPropertyMappingAttribute in the <paramref name="epmInfo"/>
        /// </summary>
        /// <param name="epmInfo">EnitityPropertyMappingInfo holding the target path</param>
        internal void Add(EntityPropertyMappingInfo epmInfo)
        {
            String targetName = epmInfo.Attribute.TargetPath;
            bool isSyndication = epmInfo.Attribute.TargetSyndicationItem != SyndicationItemProperty.CustomProperty;
            String namespaceUri = epmInfo.Attribute.TargetNamespaceUri;
            String namespacePrefix = epmInfo.Attribute.TargetNamespacePrefix;

            EpmTargetPathSegment currentSegment = isSyndication ? this.SyndicationRoot : this.NonSyndicationRoot;
            IList<EpmTargetPathSegment> activeSubSegments = currentSegment.SubSegments;

            Debug.Assert(!String.IsNullOrEmpty(targetName), "Must have been validated during EntityPropertyMappingAttribute construction");
            String[] targetSegments = targetName.Split('/');
            
            for (int i = 0; i < targetSegments.Length; i++)
            {
                String targetSegment = targetSegments[i];

                if (targetSegment.Length == 0)
                {
                    throw new InvalidOperationException(Strings.EpmTargetTree_InvalidTargetPath(targetName));
                }

                if (targetSegment[0] == '@' && i != targetSegments.Length - 1)
                {
                    throw new InvalidOperationException(Strings.EpmTargetTree_AttributeInMiddle(targetSegment));
                }

                EpmTargetPathSegment foundSegment = activeSubSegments.SingleOrDefault(
                                                        segment => segment.SegmentName == targetSegment &&
                                                        (isSyndication || segment.SegmentNamespaceUri == namespaceUri));
                if (foundSegment != null)
                {
                    currentSegment = foundSegment;
                }
                else
                {
                    currentSegment = new EpmTargetPathSegment(targetSegment, namespaceUri, namespacePrefix, currentSegment);
                    if (targetSegment[0] == '@')
                    {
                        activeSubSegments.Insert(0, currentSegment);
                    }
                    else
                    {
                        activeSubSegments.Add(currentSegment);
                    }
                }

                activeSubSegments = currentSegment.SubSegments;
            }

            // Two EpmAttributes with same TargetName in the inheritance hierarchy
            if (currentSegment.HasContent)
            {
                throw new ArgumentException(Strings.EpmTargetTree_DuplicateEpmAttrsWithSameTargetName(EpmTargetTree.GetPropertyNameFromEpmInfo(currentSegment.EpmInfo), currentSegment.EpmInfo.DefiningType.Name, currentSegment.EpmInfo.Attribute.SourcePath, epmInfo.Attribute.SourcePath));
            }

            // Increment the number of properties for which KeepInContent is false
            if (!epmInfo.Attribute.KeepInContent)
            {
                this.countOfNonContentProperties++;
            }

            currentSegment.EpmInfo = epmInfo;
            
            // Mixed content is dis-allowed. Since root has no ancestor, pass in false for ancestorHasContent
            if (EpmTargetTree.HasMixedContent(this.NonSyndicationRoot, false))
            {
                throw new InvalidOperationException(Strings.EpmTargetTree_InvalidTargetPath(targetName));
            }
        }

        /// <summary>
        /// Removes a path in the tree which is obtained by looking at the EntityPropertyMappingAttribute in the <paramref name="epmInfo"/>
        /// </summary>
        /// <param name="epmInfo">EnitityPropertyMappingInfo holding the target path</param>
        internal void Remove(EntityPropertyMappingInfo epmInfo)
        {
            String targetName = epmInfo.Attribute.TargetPath;
            bool isSyndication = epmInfo.Attribute.TargetSyndicationItem != SyndicationItemProperty.CustomProperty;
            String namespaceUri = epmInfo.Attribute.TargetNamespaceUri;

            EpmTargetPathSegment currentSegment = isSyndication ? this.SyndicationRoot : this.NonSyndicationRoot;
            List<EpmTargetPathSegment> activeSubSegments = currentSegment.SubSegments;

            Debug.Assert(!String.IsNullOrEmpty(targetName), "Must have been validated during EntityPropertyMappingAttribute construction");
            String[] targetSegments = targetName.Split('/');
            for (int i = 0; i < targetSegments.Length; i++)
            {
                String targetSegment = targetSegments[i];

                if (targetSegment.Length == 0)
                {
                    throw new InvalidOperationException(Strings.EpmTargetTree_InvalidTargetPath(targetName));
                }

                if (targetSegment[0] == '@' && i != targetSegments.Length - 1)
                {
                    throw new InvalidOperationException(Strings.EpmTargetTree_AttributeInMiddle(targetSegment));
                }

                EpmTargetPathSegment foundSegment = activeSubSegments.FirstOrDefault(
                                                        segment => segment.SegmentName == targetSegment &&
                                                        (isSyndication || segment.SegmentNamespaceUri == namespaceUri));
                if (foundSegment != null)
                {
                    currentSegment = foundSegment;
                }
                else
                {
                    return;
                }

                activeSubSegments = currentSegment.SubSegments;
            }

            // Recursively remove all the parent segments which will have no more children left 
            // after removal of the current segment node 
            if (currentSegment.HasContent)
            {
                // Since we are removing a property with KeepInContent false, we should decrement the count
                if (!currentSegment.EpmInfo.Attribute.KeepInContent)
                {
                    this.countOfNonContentProperties--;
                }

                do
                {
                    EpmTargetPathSegment parentSegment = currentSegment.ParentSegment;
                    parentSegment.SubSegments.Remove(currentSegment);
                    currentSegment = parentSegment;
                }
                while (currentSegment.ParentSegment != null && !currentSegment.HasContent && currentSegment.SubSegments.Count == 0);
            }
        }
        
        /// <summary>Checks if mappings could potentially result in mixed content and dis-allows it.</summary>
        /// <param name="currentSegment">Segment being processed.</param>
        /// <param name="ancestorHasContent">Does any of the ancestors have content.</param>
        /// <returns>boolean indicating if the tree is valid or not.</returns>
        private static bool HasMixedContent(EpmTargetPathSegment currentSegment, bool ancestorHasContent)
        {
            foreach (EpmTargetPathSegment childSegment in currentSegment.SubSegments.Where(s => !s.IsAttribute))
            {
                if (childSegment.HasContent && ancestorHasContent)
                {
                    return true;
                }
            
                if (HasMixedContent(childSegment, childSegment.HasContent || ancestorHasContent))
                {
                    return true;
                }
            }
            
            return false;
        }
        
        /// <summary>
        /// Given an <see cref="EntityPropertyMappingInfo"/> gives the correct target path for it
        /// </summary>
        /// <param name="epmInfo">Given <see cref="EntityPropertyMappingInfo"/></param>
        /// <returns>String with the correct value for the target path</returns>
        private static String GetPropertyNameFromEpmInfo(EntityPropertyMappingInfo epmInfo)
        {
#if ASTORIA_SERVER
            if (epmInfo.IsEFProvider)
            {
                if (epmInfo.Attribute.TargetSyndicationItem != SyndicationItemProperty.CustomProperty)
                {
                    return System.Data.Services.Providers.ObjectContextServiceProvider.MapSyndicationPropertyToEpmTargetPath(epmInfo.Attribute.TargetSyndicationItem);
                }
                else
                {
                    return epmInfo.Attribute.TargetPath;
                }
            }
            else
#endif
            {
                if (epmInfo.Attribute.TargetSyndicationItem != SyndicationItemProperty.CustomProperty)
                {
                    return epmInfo.Attribute.TargetSyndicationItem.ToString();
                }
                else
                {
                    return epmInfo.Attribute.TargetPath;
                }
            }
        }
    }
}
