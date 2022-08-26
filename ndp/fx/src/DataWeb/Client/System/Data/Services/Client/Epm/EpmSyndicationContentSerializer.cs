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

namespace System.Data.Services.Client
{
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Xml;

    /// <summary>
    /// Base visitor class for performing serialization of syndication specific content in the feed entry whose mapping
    /// is provided through EntityPropertyMappingAttributes
    /// </summary>
    internal sealed class EpmSyndicationContentSerializer : EpmContentSerializerBase, IDisposable
    {
        /// <summary>
        /// Is author mapping provided in the target tree
        /// </summary>
        private bool authorInfoPresent;

        /// <summary>
        /// Is updated mapping provided in the target tree
        /// </summary>
        private bool updatedPresent;

        /// <summary>
        /// Is author name provided in the target tree
        /// </summary>
        private bool authorNamePresent;

        /// <summary>
        /// Constructor initializes the base class be identifying itself as a syndication serializer
        /// </summary>
        /// <param name="tree">Target tree containing mapping information</param>
        /// <param name="element">Object to be serialized</param>
        /// <param name="target">SyndicationItem to which content will be added</param>
        internal EpmSyndicationContentSerializer(EpmTargetTree tree, object element, XmlWriter target)
            : base(tree, true, element, target)
        {
        }

        /// <summary>
        /// Used for housecleaning once every node has been visited, creates an author and updated syndication elements if none 
        /// have been created by the visitor
        /// </summary>
        public void Dispose()
        {
            this.CreateAuthor(true);
            this.CreateUpdated();
        }

        /// <summary>
        /// Override of the base Visitor method, which actually performs mapping search and serialization
        /// </summary>
        /// <param name="targetSegment">Current segment being checked for mapping</param>
        /// <param name="kind">Which sub segments to serialize</param>
        protected override void Serialize(EpmTargetPathSegment targetSegment, EpmSerializationKind kind)
        {
            if (targetSegment.HasContent)
            {
                EntityPropertyMappingInfo epmInfo = targetSegment.EpmInfo;
                Object propertyValue;

                try
                {
                    propertyValue = epmInfo.ReadPropertyValue(this.Element);
                }
                catch (System.Reflection.TargetInvocationException)
                {
                    throw;
                }

                String contentType;
                Action<String> contentWriter;

                switch (epmInfo.Attribute.TargetTextContentKind)
                {
                    case SyndicationTextContentKind.Html:
                        contentType = "html";
                        contentWriter = this.Target.WriteString;
                        break;
                    case SyndicationTextContentKind.Xhtml:
                        contentType = "xhtml";
                        contentWriter = this.Target.WriteRaw;
                        break;
                    default:
                        contentType = "text";
                        contentWriter = this.Target.WriteString;
                        break;
                }

                Action<String, bool, bool> textSyndicationWriter = (c, nonTextPossible, atomDateConstruct) =>
                {
                    this.Target.WriteStartElement(c, XmlConstants.AtomNamespace);
                    if (nonTextPossible)
                    {
                        this.Target.WriteAttributeString(XmlConstants.AtomTypeAttributeName, String.Empty, contentType);
                    }

                    // As per atomPub spec dateConstructs must contain valid datetime. Therefore we need to fill atom:updated/atom:published 
                    // field with something (e.g. DateTimeOffset.MinValue) if the user wants to map null to either of these fields. This will 
                    // satisfy protocol requirements and Syndication API. Note that the content will still contain information saying that the 
                    // mapped property is supposed to be null - therefore the server will know that the actual value sent by the user is null.
                    // For all other elements we can use empty string since the content is not validated.
                    String textPropertyValue = 
                        propertyValue != null   ? ClientConvert.ToString(propertyValue, atomDateConstruct) :
                        atomDateConstruct       ? ClientConvert.ToString(DateTime.MinValue, atomDateConstruct) : 
                        String.Empty;

                    contentWriter(textPropertyValue);
                    this.Target.WriteEndElement();
                };

                switch (epmInfo.Attribute.TargetSyndicationItem)
                {
                    case SyndicationItemProperty.AuthorEmail:
                    case SyndicationItemProperty.ContributorEmail:
                        textSyndicationWriter(XmlConstants.AtomEmailElementName, false, false);
                        break;
                    case SyndicationItemProperty.AuthorName:
                    case SyndicationItemProperty.ContributorName:
                        textSyndicationWriter(XmlConstants.AtomNameElementName, false, false);
                        this.authorNamePresent = true;
                        break;
                    case SyndicationItemProperty.AuthorUri:
                    case SyndicationItemProperty.ContributorUri:
                        textSyndicationWriter(XmlConstants.AtomUriElementName, false, false);
                        break;
                    case SyndicationItemProperty.Updated:
                        textSyndicationWriter(XmlConstants.AtomUpdatedElementName, false, true);
                        this.updatedPresent = true;
                        break;
                    case SyndicationItemProperty.Published:
                        textSyndicationWriter(XmlConstants.AtomPublishedElementName, false, true);
                        break;
                    case SyndicationItemProperty.Rights:
                        textSyndicationWriter(XmlConstants.AtomRightsElementName, true, false);
                        break;
                    case SyndicationItemProperty.Summary:
                        textSyndicationWriter(XmlConstants.AtomSummaryElementName, true, false);
                        break;
                    case SyndicationItemProperty.Title:
                        textSyndicationWriter(XmlConstants.AtomTitleElementName, true, false);
                        break;
                    default:
                        Debug.Assert(false, "Unhandled SyndicationItemProperty enum value - should never get here.");
                        break;
                }
            }
            else
            {
                if (targetSegment.SegmentName == XmlConstants.AtomAuthorElementName)
                {
                    this.CreateAuthor(false);
                    base.Serialize(targetSegment, kind);
                    this.FinishAuthor();
                }
                else if (targetSegment.SegmentName == XmlConstants.AtomContributorElementName)
                {
                    this.Target.WriteStartElement(XmlConstants.AtomContributorElementName, XmlConstants.AtomNamespace);
                    base.Serialize(targetSegment, kind);
                    this.Target.WriteEndElement();
                }
                else
                {
                    Debug.Assert(false, "Only authors and contributors have nested elements");
                }
            }
        }

        /// <summary>
        /// Creates a new author syndication specific object
        /// </summary>
        /// <param name="createNull">Whether to create a null author or a non-null author</param>
        private void CreateAuthor(bool createNull)
        {
            if (!this.authorInfoPresent)
            {
                if (createNull)
                {
                    this.Target.WriteStartElement(XmlConstants.AtomAuthorElementName, XmlConstants.AtomNamespace);
                    this.Target.WriteElementString(XmlConstants.AtomNameElementName, XmlConstants.AtomNamespace, String.Empty);
                    this.Target.WriteEndElement();
                }
                else
                {
                    this.Target.WriteStartElement(XmlConstants.AtomAuthorElementName, XmlConstants.AtomNamespace);
                }

                this.authorInfoPresent = true;
            }
        }

        /// <summary>
        /// Finish writing the author atom element
        /// </summary>
        private void FinishAuthor()
        {
            Debug.Assert(this.authorInfoPresent == true, "Must have already written the start element for author");
            if (this.authorNamePresent == false)
            {
                this.Target.WriteElementString(XmlConstants.AtomNameElementName, XmlConstants.AtomNamespace, String.Empty);
                this.authorNamePresent = true;
            }
 
            this.Target.WriteEndElement();
        }

        /// <summary>
        /// Ensures that mandatory atom:update element is creatd. The value of the created element is set to with value set to DateTime.UtcNow. 
        /// This method is used only if there are some EPM mappings for the given resource type but there is no mapping to atom:updated 
        /// element. Otherwise the element will be created during mapping handling or if there is no EPM mapping at all during the serialization. 
        /// </summary>
        private void CreateUpdated()
        {
            if (!this.updatedPresent)
            {
                this.Target.WriteElementString(XmlConstants.AtomUpdatedElementName, XmlConstants.AtomNamespace, XmlConvert.ToString(DateTime.UtcNow, XmlDateTimeSerializationMode.RoundtripKind));
            }
        }
    }
}
