//---------------------------------------------------------------------
// <copyright file="EpmSyndicationContentSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Classes used for serializing EntityPropertyMappingAttribute content for
// syndication specific mappings
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
#region Namespaces
    using System.Data.Services.Providers;
    using System.Data.Services.Serializers;
    using System.Globalization;
    using System.ServiceModel.Syndication;
    using System.Diagnostics;
#endregion

    /// <summary>
    /// Base visitor class for performing serialization of syndication specific content in the feed entry whose mapping
    /// is provided through EntityPropertyMappingAttributes
    /// </summary>
    internal sealed class EpmSyndicationContentSerializer : EpmContentSerializerBase, IDisposable
    {
        /// <summary>
        /// Syndication author object resulting from mapping
        /// </summary>
        private SyndicationPerson author;

        /// <summary>
        /// Is author mapping provided in the target tree
        /// </summary>
        private bool authorInfoPresent;

        /// <summary>
        /// Is author name provided in the target tree
        /// </summary>
        private bool authorNamePresent;

        /// <summary>Collection of null valued properties for this current serialization</summary>
        private EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties;

        /// <summary>
        /// Constructor initializes the base class be identifying itself as a syndication serializer
        /// </summary>
        /// <param name="tree">Target tree containing mapping information</param>
        /// <param name="element">Object to be serialized</param>
        /// <param name="target">SyndicationItem to which content will be added</param>
        /// <param name="nullValuedProperties">Null valued properties found during serialization</param>
        internal EpmSyndicationContentSerializer(EpmTargetTree tree, object element, SyndicationItem target, EpmContentSerializer.EpmNullValuedPropertyTree nullValuedProperties)
            : base(tree, true, element, target)
        {
            this.nullValuedProperties = nullValuedProperties;
        }

        /// <summary>
        /// Used for housecleaning once every node has been visited, creates an author syndication element if none has been
        /// created by the visitor
        /// </summary>
        public void Dispose()
        {
            this.CreateAuthor(true);
            
            if (!this.authorNamePresent)
            {
                this.author.Name = String.Empty;
                this.authorNamePresent = true;
            }
            
            this.Target.Authors.Add(this.author);
        }

        /// <summary>
        /// Override of the base Visitor method, which actually performs mapping search and serialization
        /// </summary>
        /// <param name="targetSegment">Current segment being checked for mapping</param>
        /// <param name="kind">Which sub segments to serialize</param>
        /// <param name="provider">Data Service provider used for rights verification.</param>
        protected override void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind, DataServiceProviderWrapper provider)
        {
            if (targetSegment.HasContent)
            {
                EntityPropertyMappingInfo epmInfo = targetSegment.EpmInfo;
                Object propertyValue;
                
                try
                {
                    propertyValue = epmInfo.ReadPropertyValue(this.Element, provider);
                }
                catch (System.Reflection.TargetInvocationException e)
                {
                    ErrorHandler.HandleTargetInvocationException(e);
                    throw;
                }

                if (propertyValue == null)
                {
                    this.nullValuedProperties.Add(epmInfo);
                }

                String textPropertyValue = propertyValue != null ? PlainXmlSerializer.PrimitiveToString(propertyValue) : String.Empty;
                TextSyndicationContentKind contentKind = (TextSyndicationContentKind)epmInfo.Attribute.TargetTextContentKind;

                switch (epmInfo.Attribute.TargetSyndicationItem)
                {
                    case SyndicationItemProperty.AuthorEmail:
                        this.CreateAuthor(false);
                        this.author.Email = textPropertyValue;
                        break;
                    case SyndicationItemProperty.AuthorName:
                        this.CreateAuthor(false);
                        this.author.Name = textPropertyValue;    
                        this.authorNamePresent = true;
                        break;
                    case SyndicationItemProperty.AuthorUri:
                        this.CreateAuthor(false);
                        this.author.Uri = textPropertyValue;
                        break;
                    case SyndicationItemProperty.ContributorEmail:
                    case SyndicationItemProperty.ContributorName:
                    case SyndicationItemProperty.ContributorUri:
                        this.SetContributorProperty(epmInfo.Attribute.TargetSyndicationItem, textPropertyValue);
                        break;
                    case SyndicationItemProperty.Updated:
                        this.Target.LastUpdatedTime = EpmSyndicationContentSerializer.GetDate(propertyValue, Strings.EpmSerializer_UpdatedHasWrongType);
                        break;
                    case SyndicationItemProperty.Published:
                        this.Target.PublishDate = EpmSyndicationContentSerializer.GetDate(propertyValue, Strings.EpmSerializer_PublishedHasWrongType);
                        break;
                    case SyndicationItemProperty.Rights:
                        this.Target.Copyright = new TextSyndicationContent(textPropertyValue, contentKind);
                        break;
                    case SyndicationItemProperty.Summary:
                        this.Target.Summary = new TextSyndicationContent(textPropertyValue, contentKind);
                        break;
                    case SyndicationItemProperty.Title:
                        this.Target.Title = new TextSyndicationContent(textPropertyValue, contentKind);
                        break;
                    default:
                        Debug.Fail("Unhandled SyndicationItemProperty enum value - should never get here.");
                        break;
                }
            }
            else
            {
                base.Serialize(targetSegment, kind, provider);
            }
        }

        /// <summary>
        /// Given an object returns the corresponding DateTimeOffset value through conversions
        /// </summary>
        /// <param name="propertyValue">Object containing property value</param>
        /// <param name="exceptionMsg">Message of the thrown exception if the <paramref name="propertyValue"/> cannot be converted to DateTimeOffset.</param>
        /// <returns>DateTimeOffset after conversion</returns>
        private static DateTimeOffset GetDate(object propertyValue, string exceptionMsg)
        {
            if (propertyValue == null)
            {
                return DateTimeOffset.Now;
            }

            DateTimeOffset date;

            if (propertyValue is DateTimeOffset)
            {
                date = (DateTimeOffset)propertyValue;
            }
            else if (propertyValue is DateTime)
            {
                // DateTimeOffset takes care of DateTimes of Unspecified kind so we won't end up 
                // with datetime without timezone info mapped to atom:updated or atom:published element.
                date = new DateTimeOffset((DateTime)propertyValue);
            }
            else if (propertyValue is String)
            {
                if (!DateTimeOffset.TryParse((String)propertyValue, out date))
                {
                    DateTime result;
                    if (!DateTime.TryParse((String)propertyValue, out result))
                    {
                        throw new DataServiceException(500, null, exceptionMsg, null, null);
                    }

                    date = new DateTimeOffset(result);
                }
            }
            else
            {
                try
                {
                    date = new DateTimeOffset(Convert.ToDateTime(propertyValue, CultureInfo.InvariantCulture));
                }
                catch (Exception e)
                {
                    throw new DataServiceException(500, null, exceptionMsg, null, e);
                }
            }

            return date;
        }

        /// <summary>
        /// Creates a new author syndication specific object
        /// </summary>
        /// <param name="createNull">Whether to create a null author or a non-null author</param>
        private void CreateAuthor(bool createNull)
        {
            if (!this.authorInfoPresent)
            {
                this.author = createNull ? new SyndicationPerson(null, String.Empty, null) : new SyndicationPerson();
                this.authorInfoPresent = true;
            }
        }

        /// <summary>
        /// Set a property of atom:contributor element. atom:contributor is created if does not exist.
        /// </summary>
        /// <param name="propertyToSet">Property to be set.</param>
        /// <param name="textPropertyValue">Value of the property.</param>
        private void SetContributorProperty(SyndicationItemProperty propertyToSet, string textPropertyValue)
        {
            if (this.Target.Contributors.Count == 0) 
            {
                this.Target.Contributors.Add(new SyndicationPerson());
            }

            // We can have at most one contributor
            switch (propertyToSet) 
            {
                case SyndicationItemProperty.ContributorEmail:
                    this.Target.Contributors[0].Email = textPropertyValue;
                    break;
                case SyndicationItemProperty.ContributorName:
                    this.Target.Contributors[0].Name = textPropertyValue;
                    break;
                case SyndicationItemProperty.ContributorUri:
                    this.Target.Contributors[0].Uri = textPropertyValue;
                    break;
                default:
                    Debug.Fail("propertyToSet is not a Contributor property.");
                    break;
            }

            Debug.Assert(this.Target.Contributors.Count == 1, "There should be one and only one contributor.");
        }
    }
}
