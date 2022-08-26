//---------------------------------------------------------------------
// <copyright file="EpmContentSerializerBase.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Base Classes used for EntityPropertyMappingAttribute related content 
// serializers
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
    using System.Xml;
#else
    using System.ServiceModel.Syndication;
    using System.Data.Services.Providers;
#endif

    /// <summary>
    /// What kind of xml nodes to serialize
    /// </summary>
    internal enum EpmSerializationKind
    {
        /// <summary>
        /// Serialize only attributes
        /// </summary>
        Attributes,
        
        /// <summary>
        /// Serialize only elements
        /// </summary>
        Elements,
        
        /// <summary>
        /// Serialize everything
        /// </summary>
        All
    }
    
    /// <summary>
    /// Base visitor class for performing serialization of content whose location in the syndication
    /// feed is provided through EntityPropertyMappingAttributes
    /// </summary>
    internal abstract class EpmContentSerializerBase
    {
        /// <summary>
        /// Constructor decided whether to use syndication or non-syndication sub-tree for target content mappings
        /// </summary>
        /// <param name="tree">Target tree containing mapping information</param>
        /// <param name="isSyndication">Helps in deciding whether to use syndication sub-tree or non-syndication one</param>
        /// <param name="element">Object to be serialized</param>
        /// <param name="target">SyndicationItem to which content will be added</param>
#if ASTORIA_CLIENT
        protected EpmContentSerializerBase(EpmTargetTree tree, bool isSyndication, object element, XmlWriter target)
#else
        protected EpmContentSerializerBase(EpmTargetTree tree, bool isSyndication, object element, SyndicationItem target)
#endif
        {
            this.Root = isSyndication ? tree.SyndicationRoot : tree.NonSyndicationRoot;
            this.Element = element;
            this.Target = target;
            this.Success = false;
        }

        /// <summary>Root of the target tree containing mapped xml elements/attribute</summary>
        protected EpmTargetPathSegment Root
        {
            get;
            private set;
        }

        /// <summary>Object whose properties we will read</summary>
        protected object Element
        {
            get;
            private set;
        }

        /// <summary>Target SyndicationItem on which we are going to add the serialized content</summary>
#if ASTORIA_CLIENT
        protected XmlWriter Target
#else
        protected SyndicationItem Target
#endif
        {
            get;
            private set;
        }

        /// <summary>Indicates the success or failure of serialization</summary>
        protected bool Success
        {
            get;
            private set;
        }

#if ASTORIA_CLIENT
        /// <summary>Public interface used by the EpmContentSerializer class</summary>
        internal void Serialize()
#else
        /// <summary>Public interface used by the EpmContentSerializer class</summary>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        internal void Serialize(DataServiceProviderWrapper provider)
#endif
        {
            foreach (EpmTargetPathSegment targetSegment in this.Root.SubSegments)
            {
#if ASTORIA_CLIENT
                this.Serialize(targetSegment, EpmSerializationKind.All);
#else
                this.Serialize(targetSegment, EpmSerializationKind.All, provider);
#endif
            }
            
            this.Success = true;
        }

#if ASTORIA_CLIENT
        /// <summary>
        /// Internal interface to be overridden in the subclasses. 
        /// Goes through each subsegments and invokes itself for the children
        /// </summary>
        /// <param name="targetSegment">Current root segment in the target tree</param>
        /// <param name="kind">Which sub segments to serialize</param>
        protected virtual void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind)
#else
        /// <summary>
        /// Internal interface to be overridden in the subclasses. 
        /// Goes through each subsegments and invokes itself for the children
        /// </summary>
        /// <param name="targetSegment">Current root segment in the target tree</param>
        /// <param name="kind">Which sub segments to serialize</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        protected virtual void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind, DataServiceProviderWrapper provider)
#endif
        {
            IEnumerable<EpmTargetPathSegment> segmentsToSerialize;
            switch (kind)
            {
                case EpmSerializationKind.Attributes:
                    segmentsToSerialize = targetSegment.SubSegments.Where(s => s.IsAttribute == true);
                    break;
                case EpmSerializationKind.Elements:
                    segmentsToSerialize = targetSegment.SubSegments.Where(s => s.IsAttribute == false);
                    break;
                default:
                    Debug.Assert(kind == EpmSerializationKind.All, "Must serialize everything");
                    segmentsToSerialize = targetSegment.SubSegments;
                    break;
            }

            foreach (EpmTargetPathSegment segment in segmentsToSerialize)
            {
#if ASTORIA_CLIENT
                this.Serialize(segment, kind);
#else
                this.Serialize(segment, kind, provider);
#endif
            }
        }
    }
}
