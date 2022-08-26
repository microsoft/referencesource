//---------------------------------------------------------------------
// <copyright file="EntityPropertyMappingAttribute.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      EntityPropertyMapping attribute implementation
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Common
{
    using System;
    using System.Data.Services.Client;

    /// <summary>
    /// List of syndication properties settable through <see cref="EntityPropertyMappingAttribute"/>
    /// </summary>
    /// <remarks>
    /// Potentially the following atom specific elements could also be considered:
    /// * Author*
    /// * Category*
    /// * Content?
    /// * Contributor*
    /// * Id
    /// * Link*
    /// * Source?
    /// </remarks>
    public enum SyndicationItemProperty
    {
        /// <summary>
        /// User specified a non-syndication property
        /// </summary>
        CustomProperty,

        /// <summary>
        /// author/email
        /// </summary>
        AuthorEmail,

        /// <summary>
        /// author/name
        /// </summary>
        AuthorName,         // Author*

        /// <summary>
        /// author/uri
        /// </summary>
        AuthorUri,

        /// <summary>
        /// contributor/email
        /// </summary>
        ContributorEmail,

        /// <summary>
        /// contributor/name
        /// </summary>
        ContributorName,    // Contributor*

        /// <summary>
        /// contributor/uri
        /// </summary>
        ContributorUri,

        /// <summary>
        /// updated
        /// </summary>
        Updated,

        // CategoryTerm,     // Category*
        // CategoryScheme,
        // CategoryLabel,
        // LinkHref,         // Link*
        // LinkRel,
        // LinkType,
        // LinkHrefLang,
        // LinkTitle,
        // LinkLength,

        /// <summary>
        /// published
        /// </summary>
        Published,

        /// <summary>
        /// rights
        /// </summary>
        Rights,

        /// <summary>
        /// summary
        /// </summary>
        Summary,

        /// <summary>
        /// title
        /// </summary>
        Title
    }

    /// <summary>
    /// Type of content for a <see cref="SyndicationItemProperty"/> if the property
    /// is of text type
    /// </summary>
    public enum SyndicationTextContentKind
    {
        /// <summary>
        /// Plaintext
        /// </summary>
        Plaintext,

        /// <summary>
        /// HTML
        /// </summary>
        Html,

        /// <summary>
        /// XHTML
        /// </summary>
        Xhtml
    }

    /// <summary>
    /// Attribute used for mapping a given property or sub-property of a ResourceType to
    /// an xml element/attribute with arbitrary nesting
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true, Inherited = true)]
    public sealed class EntityPropertyMappingAttribute : Attribute
    {
#region Private Members

        /// <summary> Schema Namespace prefix for atom.</summary>
        private const string AtomNamespacePrefix = "atom";

        /// <summary>
        /// Source property path
        /// </summary>
        private readonly String sourcePath;

        /// <summary>
        /// Target Xml element/attribute
        /// </summary>
        private readonly String targetPath;

        /// <summary>
        /// If mapping to syndication element, the name of syndication item
        /// </summary>
        private readonly SyndicationItemProperty targetSyndicationItem;

        /// <summary>
        /// If mapping to syndication content, the content type of syndication item
        /// </summary>
        private readonly SyndicationTextContentKind targetTextContentKind;

        /// <summary>
        /// If mapping to non-syndication element/attribute, the namespace prefix for the 
        /// target element/attribute
        /// </summary>
        private readonly String targetNamespacePrefix;

        /// <summary>
        /// If mapping to non-syndication element/attribute, the namespace for the 
        /// target element/attribute
        /// </summary>
        private readonly String targetNamespaceUri;

        /// <summary>
        /// The content can optionally be kept in the original location along with the 
        /// newly mapping location by setting this option to true, false by default
        /// </summary>
        private readonly bool keepInContent;
#endregion

#region Constructors

        /// <summary>
        /// Used for mapping a resource property to syndication content
        /// </summary>
        /// <param name="sourcePath">Source property path</param>
        /// <param name="targetSyndicationItem">Syndication item to which the <see cref="sourcePath"/> is mapped</param>
        /// <param name="targetTextContentKind">Syndication content kind for <see cref="targetSyndicationItem"/></param>
        /// <param name="keepInContent">If true the property value is kept in the content section as before, 
        /// when false the property value is only placed at the mapped location</param>
        public EntityPropertyMappingAttribute(String sourcePath, SyndicationItemProperty targetSyndicationItem, SyndicationTextContentKind targetTextContentKind, bool keepInContent)
        {
            if (String.IsNullOrEmpty(sourcePath))
            {
                throw new ArgumentException(Strings.EntityPropertyMapping_EpmAttribute("sourcePath"));
            }

            this.sourcePath            = sourcePath;
            this.targetPath            = SyndicationItemPropertyToPath(targetSyndicationItem);
            this.targetSyndicationItem = targetSyndicationItem;
            this.targetTextContentKind = targetTextContentKind;
            this.targetNamespacePrefix = EntityPropertyMappingAttribute.AtomNamespacePrefix;
            this.targetNamespaceUri    = XmlConstants.AtomNamespace;
            this.keepInContent         = keepInContent;
        }

        /// <summary>
        /// Used for mapping a resource property to arbitrary xml element/attribute
        /// </summary>
        /// <param name="sourcePath">Source property path</param>
        /// <param name="targetPath">Target element/attribute path</param>
        /// <param name="targetNamespacePrefix">Namespace prefix for the <see cref="targetNamespaceUri"/> to which <see cref="targetPath"/> belongs</param>
        /// <param name="targetNamespaceUri">Uri of the namespace to which <see cref="targetPath"/> belongs</param>
        /// <param name="keepInContent">If true the property value is kept in the content section as before, 
        /// when false the property value is only placed at the mapped location</param>
        public EntityPropertyMappingAttribute(String sourcePath, String targetPath, String targetNamespacePrefix, String targetNamespaceUri, bool keepInContent)
        {
            if (String.IsNullOrEmpty(sourcePath))
            {
                throw new ArgumentException(Strings.EntityPropertyMapping_EpmAttribute("sourcePath"));
            }

            this.sourcePath = sourcePath;

            if (String.IsNullOrEmpty(targetPath))
            {
                throw new ArgumentException(Strings.EntityPropertyMapping_EpmAttribute("targetPath"));
            }

            if (targetPath[0] == '@')
            {
                throw new ArgumentException(Strings.EpmTargetTree_InvalidTargetPath(targetPath));
            }

            this.targetPath = targetPath;

            this.targetSyndicationItem = SyndicationItemProperty.CustomProperty;
            this.targetTextContentKind = SyndicationTextContentKind.Plaintext;
            this.targetNamespacePrefix = targetNamespacePrefix;

            if (String.IsNullOrEmpty(targetNamespaceUri))
            {
                throw new ArgumentException(Strings.EntityPropertyMapping_EpmAttribute("targetNamespaceUri"));
            }

            this.targetNamespaceUri = targetNamespaceUri;

            Uri uri;
            if (!Uri.TryCreate(targetNamespaceUri, UriKind.Absolute, out uri))
            {
                throw new ArgumentException(Strings.EntityPropertyMapping_TargetNamespaceUriNotValid(targetNamespaceUri));
            }

            this.keepInContent = keepInContent;
        }

        #region Properties

        /// <summary>
        /// Source property path
        /// </summary>
        public String SourcePath
        {
            get { return this.sourcePath; }
        }

        /// <summary>
        /// Target Xml element/attribute
        /// </summary>
        public String TargetPath
        {
            get { return this.targetPath; }
        }

        /// <summary>
        /// If mapping to syndication element, the name of syndication item
        /// </summary>
        public SyndicationItemProperty TargetSyndicationItem
        {
            get { return this.targetSyndicationItem; }
        }

        /// <summary>
        /// If mapping to non-syndication element/attribute, the namespace prefix for the 
        /// target element/attribute
        /// </summary>
        public String TargetNamespacePrefix
        {
            get { return this.targetNamespacePrefix; }
        }

        /// <summary>
        /// If mapping to non-syndication element/attribute, the namespace for the 
        /// target element/attribute
        /// </summary>
        public String TargetNamespaceUri
        {
            get { return this.targetNamespaceUri; }
        }

        /// <summary>
        /// If mapping to syndication content, the content type of syndication item
        /// </summary>
        public SyndicationTextContentKind TargetTextContentKind
        {
            get { return this.targetTextContentKind; }
        }

        /// <summary>
        /// The content can optionally be kept in the original location along with the 
        /// newly mapping location by setting this option to true, false by default
        /// </summary>
        public bool KeepInContent
        {
            get { return this.keepInContent; }
        }

        #endregion

        /// <summary>
        /// Maps the enumeration of allowed <see cref="SyndicationItemProperty"/> values to their string representations
        /// </summary>
        /// <param name="targetSyndicationItem">Value of the <see cref="SyndicationItemProperty"/> given in 
        /// the <see cref="EntityPropertyMappingAttribute"/> contstructor</param>
        /// <returns>String representing the xml element path in the syndication property</returns>
        internal static String SyndicationItemPropertyToPath(SyndicationItemProperty targetSyndicationItem)
        {
            switch (targetSyndicationItem)
            {
                case SyndicationItemProperty.AuthorEmail:
                    return "author/email";
                case SyndicationItemProperty.AuthorName:
                    return "author/name";
                case SyndicationItemProperty.AuthorUri:
                    return "author/uri";
                case SyndicationItemProperty.ContributorEmail:
                    return "contributor/email";
                case SyndicationItemProperty.ContributorName:
                    return "contributor/name";
                case SyndicationItemProperty.ContributorUri:
                    return "contributor/uri";
                case SyndicationItemProperty.Updated:
                    return "updated";
                case SyndicationItemProperty.Published:
                    return "published";
                case SyndicationItemProperty.Rights:
                    return "rights";
                case SyndicationItemProperty.Summary:
                    return "summary";
                case SyndicationItemProperty.Title:
                    return "title";
                default:
                    throw new ArgumentException(Strings.EntityPropertyMapping_EpmAttribute("targetSyndicationItem"));
            }
        }

#endregion 
    }
}
