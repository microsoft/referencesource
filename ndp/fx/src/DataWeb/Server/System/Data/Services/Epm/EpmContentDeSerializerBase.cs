//---------------------------------------------------------------------
// <copyright file="EpmContentDeSerializerBase.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Base Class used for EntityPropertyMappingAttribute related content 
// deserializers
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.ServiceModel.Syndication;
#endregion

    /// <summary>
    /// Base EPM deserializer class
    /// </summary>
    internal abstract class EpmContentDeSerializerBase
    {
        /// <summary><see cref="SyndicationItem"/> from which to read EPM content</summary>
        private readonly SyndicationItem item;
        
        /// <summary>Deserializer state</summary>
        private readonly EpmContentDeSerializer.EpmContentDeserializerState state;
        
        /// <summary>Constructor</summary>
        /// <param name="item"><see cref="SyndicationItem"/> from which to read EPM content</param>
        /// <param name="state">State of the deserializer</param>
        internal EpmContentDeSerializerBase(SyndicationItem item, EpmContentDeSerializer.EpmContentDeserializerState state)
        {
            this.item = item;
            this.state = state;
        }

        /// <summary>Object update interface</summary>
        internal UpdatableWrapper Updatable
        {
            get
            {
                return this.state.Updatable;
            }
        }

        /// <summary>Are we deserializing for an update operation</summary>
        internal bool IsUpdateOperation
        {
            get
            {
                return this.state.IsUpdateOperation;
            }
        }

        /// <summary>Current service instance</summary>
        internal IDataService Service
        {
            get
            {
                return this.state.Service;
            }
        }

        /// <summary>Current service instance</summary>
        internal EpmContentDeSerializer.EpmAppliedPropertyInfo PropertiesApplied
        {
            get
            {
                return this.state.PropertiesApplied;
            }
        }

        /// <summary>SyndicationItem to read EPM content from</summary>
        protected SyndicationItem Item
        {
            get
            {
                return this.item;
            }
        }

        /// <summary>
        /// Matches the targetSegment with properties already applied and if finds something already applied considers it a match
        /// </summary>
        /// <param name="targetSegment">Target segment for which existing property application is checked for</param>
        /// <param name="propertiesApplied">Properties already applied based on content</param>
        /// <returns>true if already the property for the current segment has been applied</returns>
        internal static bool Match(EpmTargetPathSegment targetSegment, EpmContentDeSerializer.EpmAppliedPropertyInfo propertiesApplied)
        {
            if (!targetSegment.EpmInfo.Attribute.KeepInContent)
            {
                return propertiesApplied.Lookup(targetSegment.EpmInfo.Attribute.SourcePath);
            }
            else
            {
                return true;
            }
        }
    }
}
