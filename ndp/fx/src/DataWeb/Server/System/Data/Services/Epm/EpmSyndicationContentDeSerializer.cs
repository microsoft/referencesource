//---------------------------------------------------------------------
// <copyright file="EpmSyndicationContentDeSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Deserializer for the EPM content on the server
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.Data.Services.Providers;
    using System.ServiceModel.Syndication;
    using System.Diagnostics;
    using System.Xml;

#endregion

    /// <summary>Syndication content reader for EPM content</summary>
    internal sealed class EpmSyndicationContentDeSerializer : EpmContentDeSerializerBase
    {
        /// <summary>Constructor</summary>
        /// <param name="item"><see cref="SyndicationItem"/> to read content from</param>
        /// <param name="state">State of the deserializer</param>
        internal EpmSyndicationContentDeSerializer(SyndicationItem item, EpmContentDeSerializer.EpmContentDeserializerState state)
        : base(item, state)
        {
        }

        /// <summary>Publicly accessible deserialization entry point</summary>
        /// <param name="resourceType">Type of resource to deserialize</param>
        /// <param name="element">Token corresponding to object of <paramref name="resourceType"/></param>
        internal void DeSerialize(ResourceType resourceType, object element)
        {
            this.DeSerialize(resourceType.EpmTargetTree.SyndicationRoot, resourceType, element);
        }

        /// <summary>Used for deserializing each of syndication specific content nodes</summary>
        /// <param name="currentRoot">Node in the target path being processed</param>
        /// <param name="resourceType">ResourceType</param>
        /// <param name="element">object being deserialized</param>
        private void DeSerialize(EpmTargetPathSegment currentRoot, ResourceType resourceType, object element)
        {
            foreach (EpmTargetPathSegment newRoot in currentRoot.SubSegments)
            {
                if (newRoot.HasContent)
                {
                    if (!EpmContentDeSerializerBase.Match(newRoot, this.PropertiesApplied))
                    {
                        switch (newRoot.EpmInfo.Attribute.TargetSyndicationItem)
                        {
                            case SyndicationItemProperty.AuthorEmail:
                                if (this.Item.Authors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Authors[0].Email, this);
                                }

                                break;
                            case SyndicationItemProperty.AuthorName:
                                if (this.Item.Authors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Authors[0].Name, this);
                                }

                                break;
                            case SyndicationItemProperty.AuthorUri:
                                if (this.Item.Authors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Authors[0].Uri, this);
                                }

                                break;
                            case SyndicationItemProperty.ContributorEmail:
                                if (this.Item.Contributors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Contributors[0].Email, this);
                                }

                                break;
                            case SyndicationItemProperty.ContributorName:
                                if (this.Item.Contributors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Contributors[0].Name, this);
                                }

                                break;
                            case SyndicationItemProperty.ContributorUri:
                                if (this.Item.Contributors.Count > 0)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Contributors[0].Uri, this);
                                }

                                break;
                            case SyndicationItemProperty.Updated:
                                // If this.Item.LastUpdatedTime == DateTimeOffset.MinValue we assume that the date has not been provided 
                                // by the user. This is the same assumption Syndication Api does (see Atom10FeedFormatter.WriteItemContents()).
                                // If the date was not provided by the user we should not touch it - otherwise the response will not be  
                                // compatible with response sent for the same request and the same resource type but having KeepInContent set to true
                                if (this.Item.LastUpdatedTime > DateTimeOffset.MinValue)
                                {
                                    resourceType.SetEpmValue(newRoot, element, XmlConvert.ToString(this.Item.LastUpdatedTime), this);
                                }

                                break;
                            case SyndicationItemProperty.Published:
                                if (this.Item.PublishDate > DateTimeOffset.MinValue)
                                {
                                    resourceType.SetEpmValue(newRoot, element, XmlConvert.ToString(this.Item.PublishDate), this);
                                }

                                break;
                            case SyndicationItemProperty.Rights:
                                if (this.Item.Copyright != null)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Copyright.Text, this);
                                }

                                break;
                            case SyndicationItemProperty.Summary:
                                if (this.Item.Summary != null)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Summary.Text, this);
                                }

                                break;
                            case SyndicationItemProperty.Title:
                                if (this.Item.Title != null)
                                {
                                    resourceType.SetEpmValue(newRoot, element, this.Item.Title.Text, this);
                                }

                                break;
                            default:
                                Debug.Fail("Unhandled SyndicationItemProperty enum value - should never get here.");
                                break;
                        }
                    }
                }
                else
                {
                    this.DeSerialize(newRoot, resourceType, element);
                }
            }
        }
    }
}
