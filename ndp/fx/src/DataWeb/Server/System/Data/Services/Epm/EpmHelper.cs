//---------------------------------------------------------------------
// <copyright file="EpmHelper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface definition for web data service
//      data sources.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

#if ASTORIA_SERVER 

namespace System.Data.Services.Providers
{
    using System.Data.Services;

#else

namespace System.Data.EntityModel.Emitters
{
    using System.Data.Services.Design;

#endif

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data;
    using System.Data.EntityClient;
    using System.Data.Metadata.Edm;
    using System.Data.Objects;
    using System.Data.Services.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// This class contains code for translating epm information stored in Metadata properties to objects of EpmPropertyInformation class
    /// !!! THIS CODE IS USED BY System.Data.Services.Providers.ObjectContextProvider *AND* System.Data.EntityModel.Emitters.AttributeEmitter CLASSES !!!
    /// </summary>
#if ASTORIA_SERVER 
    internal partial class ObjectContextServiceProvider
#else
    internal sealed partial class AttributeEmitter
#endif
    {
        /// <summary>
        /// Obtains the epm information for a single property by reading csdl content
        /// </summary>
        /// <param name="extendedProperties">Collection of extended metadata properties for a resource</param>
        /// <param name="typeName">Type for which we are reading the metadata properties</param>
        /// <param name="memberName">Member for which we are reading the metadata properties</param>
        /// <returns>EpmPropertyInformation corresponding to read metadata properties</returns>
        private static IEnumerable<EpmPropertyInformation> GetEpmPropertyInformation(IEnumerable<MetadataProperty> extendedProperties, String typeName, String memberName)
        {
            EpmAttributeNameBuilder epmAttributeNameBuilder = new EpmAttributeNameBuilder();

            while (true)
            {
                bool pathGiven = true;

                // EpmTargetPath is the only non-optional EPM attribute. If it is declared we need to take care of mapping.
                MetadataProperty epmTargetPathProperty = FindSingletonExtendedProperty(
                                                            extendedProperties,
                                                            epmAttributeNameBuilder.EpmTargetPath,
                                                            typeName,
                                                            memberName);

                if (epmTargetPathProperty != null)
                {
                    // By default, we keep the copy in content for backwards compatibility
                    bool epmKeepInContent = true;

                    MetadataProperty epmKeepInContentProperty = FindSingletonExtendedProperty(
                                                                extendedProperties,
                                                                epmAttributeNameBuilder.EpmKeepInContent,
                                                                typeName,
                                                                memberName);
                    if (epmKeepInContentProperty != null)
                    {
                        if (!Boolean.TryParse(Convert.ToString(epmKeepInContentProperty.Value, CultureInfo.InvariantCulture), out epmKeepInContent))
                        {
                            throw new InvalidOperationException(memberName == null ?
                                    Strings.ObjectContext_InvalidValueForEpmPropertyType(epmAttributeNameBuilder.EpmKeepInContent, typeName) :
                                    Strings.ObjectContext_InvalidValueForEpmPropertyMember(epmAttributeNameBuilder.EpmKeepInContent, memberName, typeName));
                        }
                    }

                    MetadataProperty epmSourcePathProperty = FindSingletonExtendedProperty(
                                                                extendedProperties,
                                                                epmAttributeNameBuilder.EpmSourcePath,
                                                                typeName,
                                                                memberName);
                    String epmSourcePath;
                    if (epmSourcePathProperty == null)
                    {
                        if (memberName == null)
                        {
                            throw new InvalidOperationException(Strings.ObjectContext_MissingExtendedAttributeType(epmAttributeNameBuilder.EpmSourcePath, typeName));
                        }

                        pathGiven = false;
                        epmSourcePath = memberName;
                    }
                    else
                    {
                        epmSourcePath = Convert.ToString(epmSourcePathProperty.Value, CultureInfo.InvariantCulture);
                    }

                    String epmTargetPath = Convert.ToString(epmTargetPathProperty.Value, CultureInfo.InvariantCulture);

                    // if the property is not a sydication property MapEpmTargetPathToSyndicationProperty
                    // will return SyndicationItemProperty.CustomProperty 
                    SyndicationItemProperty targetSyndicationItem = MapEpmTargetPathToSyndicationProperty(epmTargetPath);

                    MetadataProperty epmContentKindProperty = FindSingletonExtendedProperty(
                                                                extendedProperties,
                                                                epmAttributeNameBuilder.EpmContentKind,
                                                                typeName,
                                                                memberName);

                    MetadataProperty epmNsPrefixProperty = FindSingletonExtendedProperty(
                                                                extendedProperties,
                                                                epmAttributeNameBuilder.EpmNsPrefix,
                                                                typeName,
                                                                memberName);

                    MetadataProperty epmNsUriProperty = FindSingletonExtendedProperty(
                                                                extendedProperties,
                                                                epmAttributeNameBuilder.EpmNsUri,
                                                                typeName,
                                                                memberName);

                    // ContentKind is mutually exclusive with NsPrefix and NsUri properties
                    if (epmContentKindProperty != null)
                    {
                        if (epmNsPrefixProperty != null || epmNsUriProperty != null)
                        {
                            string epmPropertyName = epmNsPrefixProperty != null ? epmAttributeNameBuilder.EpmNsPrefix : epmAttributeNameBuilder.EpmNsUri;

                            throw new InvalidOperationException(memberName == null ?
                                            Strings.ObjectContext_InvalidAttributeForNonSyndicationItemsType(epmPropertyName, typeName) :
                                            Strings.ObjectContext_InvalidAttributeForNonSyndicationItemsMember(epmPropertyName, memberName, typeName));
                        }
                    }

                    // epmNsPrefixProperty and epmNsUriProperty can be non-null only for non-Atom mapping. Since they are optional we need to check
                    // if it was possible to map the target path to a syndication item name. if it was not (i.e. targetSyndicationItem == SyndicationItemProperty.CustomProperty) 
                    // this is a non-Atom kind of mapping.
                    if (epmNsPrefixProperty != null || epmNsUriProperty != null || targetSyndicationItem == SyndicationItemProperty.CustomProperty)
                    {
                        String epmNsPrefix = epmNsPrefixProperty != null ? Convert.ToString(epmNsPrefixProperty.Value, CultureInfo.InvariantCulture) : null;
                        String epmNsUri = epmNsUriProperty != null ? Convert.ToString(epmNsUriProperty.Value, CultureInfo.InvariantCulture) : null;
                        yield return new EpmPropertyInformation
                                            {
                                                IsAtom = false,
                                                KeepInContent = epmKeepInContent,
                                                SourcePath = epmSourcePath,
                                                PathGiven = pathGiven,
                                                TargetPath = epmTargetPath,
                                                NsPrefix = epmNsPrefix,
                                                NsUri = epmNsUri
                                            };
                    }
                    else
                    {
                        SyndicationTextContentKind syndicationContentKind;

                        if (epmContentKindProperty != null)
                        {
                            syndicationContentKind = MapEpmContentKindToSyndicationTextContentKind(
                                                        Convert.ToString(epmContentKindProperty.Value, CultureInfo.InvariantCulture),
                                                        typeName,
                                                        memberName);
                        }
                        else
                        {
                            syndicationContentKind = SyndicationTextContentKind.Plaintext;
                        }

                        yield return new EpmPropertyInformation
                                            {
                                                IsAtom = true,
                                                KeepInContent = epmKeepInContent,
                                                SourcePath = epmSourcePath,
                                                PathGiven = pathGiven,
                                                SyndicationItem = targetSyndicationItem,
                                                ContentKind = syndicationContentKind
                                            };
                    }

                    epmAttributeNameBuilder.MoveNext();
                }
                else
                {
                    yield break;
                }
            }
        }

        /// <summary>
        /// Finds the extended property from a collection of extended EFx properties, only allows singletons
        /// </summary>
        /// <param name="metadataExtendedProperties">Collection of metadata extended properties of <paramref name="memberName"/></param>
        /// <param name="propertyName">Name of the property</param>
        /// <param name="typeName">Type to which the property belongs</param>
        /// <param name="memberName">Name of the member whose extended properties we are searching from</param>
        /// <returns>The corresponding MetadataProperty object if found, null otherwise</returns>
        private static MetadataProperty FindSingletonExtendedProperty(IEnumerable<MetadataProperty> metadataExtendedProperties, String propertyName, String typeName, String memberName)
        {
            string extendedPropertyName = System.Data.Services.XmlConstants.DataWebMetadataNamespace + ":" + propertyName;
            IEnumerable<MetadataProperty> result = metadataExtendedProperties.Where(mdp => mdp.Name == extendedPropertyName);
            bool found = false;
            MetadataProperty property = null;
            foreach (MetadataProperty p in result)
            {
                if (found)
                {
                    throw new InvalidOperationException(memberName == null ?
                                Strings.ObjectContext_MultipleValuesForSameExtendedAttributeType(propertyName, typeName) :
                                Strings.ObjectContext_MultipleValuesForSameExtendedAttributeMember(propertyName, memberName, typeName));
                }

                property = p;
                found = true;
            }

            return property;
        }

        /// <summary>
        /// Given a <paramref name="targetPath"/> gets the corresponding syndication property.
        /// </summary>
        /// <param name="targetPath">Target path in the form of syndication property name</param>
        /// <returns>
        /// Enumerated value of a SyndicationItemProperty or SyndicationItemProperty.CustomProperty if the <paramref name="targetPath"/>
        /// does not map to any syndication property name.
        /// </returns>
        private static SyndicationItemProperty MapEpmTargetPathToSyndicationProperty(String targetPath)
        {
            Debug.Assert(Enum.GetNames(typeof(SyndicationItemProperty)).Count() == 12, "Any addition to SyndicationItemPropery enum requires updating this method.");

            SyndicationItemProperty targetSyndicationItem = SyndicationItemProperty.CustomProperty;

            switch (targetPath)
            {
                case System.Data.Services.XmlConstants.SyndAuthorEmail:
                    targetSyndicationItem = SyndicationItemProperty.AuthorEmail;
                    break;
                case System.Data.Services.XmlConstants.SyndAuthorName:
                    targetSyndicationItem = SyndicationItemProperty.AuthorName;
                    break;
                case System.Data.Services.XmlConstants.SyndAuthorUri:
                    targetSyndicationItem = SyndicationItemProperty.AuthorUri;
                    break;
                case System.Data.Services.XmlConstants.SyndContributorEmail:
                    targetSyndicationItem = SyndicationItemProperty.ContributorEmail;
                    break;
                case System.Data.Services.XmlConstants.SyndContributorName:
                    targetSyndicationItem = SyndicationItemProperty.ContributorName;
                    break;
                case System.Data.Services.XmlConstants.SyndContributorUri:
                    targetSyndicationItem = SyndicationItemProperty.ContributorUri;
                    break;
                case System.Data.Services.XmlConstants.SyndUpdated:
                    targetSyndicationItem = SyndicationItemProperty.Updated;
                    break;
                case System.Data.Services.XmlConstants.SyndPublished:
                    targetSyndicationItem = SyndicationItemProperty.Published;
                    break;
                case System.Data.Services.XmlConstants.SyndRights:
                    targetSyndicationItem = SyndicationItemProperty.Rights;
                    break;
                case System.Data.Services.XmlConstants.SyndSummary:
                    targetSyndicationItem = SyndicationItemProperty.Summary;
                    break;
                case System.Data.Services.XmlConstants.SyndTitle:
                    targetSyndicationItem = SyndicationItemProperty.Title;
                    break;
                default:
                    targetSyndicationItem = SyndicationItemProperty.CustomProperty;
                    break;
            }

            return targetSyndicationItem;
        }

        /// <summary>
        /// Given the string representation in <paramref name="strContentKind"/> gets back the corresponding enumerated value
        /// </summary>
        /// <param name="strContentKind">String representation of syndication content kind e.g. plaintext, html or xhtml</param>
        /// <param name="typeName">Type to which the property belongs</param>
        /// <param name="memberName">Name of the member whose extended properties we are searching from</param>
        /// <returns>Enumerated value of SyndicationTextContentKind</returns>
        private static SyndicationTextContentKind MapEpmContentKindToSyndicationTextContentKind(String strContentKind, String typeName, String memberName)
        {
            SyndicationTextContentKind contentKind = SyndicationTextContentKind.Plaintext;

            switch (strContentKind)
            {
                case System.Data.Services.XmlConstants.SyndContentKindPlaintext:
                    contentKind = SyndicationTextContentKind.Plaintext;
                    break;
                case System.Data.Services.XmlConstants.SyndContentKindHtml:
                    contentKind = SyndicationTextContentKind.Html;
                    break;
                case System.Data.Services.XmlConstants.SyndContentKindXHtml:
                    contentKind = SyndicationTextContentKind.Xhtml;
                    break;
                default:
                    throw new InvalidOperationException(memberName == null ?
                                Strings.ObjectContext_InvalidValueForTargetTextContentKindPropertyType(strContentKind, typeName) :
                                Strings.ObjectContext_InvalidValueForTargetTextContentKindPropertyMember(strContentKind, memberName, typeName));
            }

            return contentKind;
        }

        /// <summary>
        /// Class for holding de-serialized Epm attribute from csdl file
        /// </summary>
        private sealed class EpmPropertyInformation
        {
            /// <summary>Syndication mapping or custom mapping</summary>
            internal bool IsAtom
            {
                get;
                set;
            }

            /// <summary>KeepInContent</summary>
            internal bool KeepInContent
            {
                get;
                set;
            }

            /// <summary>SourcePath</summary>
            internal String SourcePath
            {
                get;
                set;
            }

            /// <summary>Was path provided or inferred</summary>
            internal bool PathGiven
            {
                get;
                set;
            }

            /// <summary>TargetPath</summary>
            internal String TargetPath
            {
                get;
                set;
            }

            /// <summary>Target syndication item when IsAtom is true</summary>
            internal SyndicationItemProperty SyndicationItem
            {
                get;
                set;
            }

            /// <summary>Target syndication item content kind when IsAtom is true</summary>
            internal SyndicationTextContentKind ContentKind
            {
                get;
                set;
            }

            /// <summary>Namespace prefix when IsAtom is false</summary>
            internal String NsPrefix
            {
                get;
                set;
            }

            /// <summary>Namespace Uri when IsAtom is false</summary>
            internal String NsUri
            {
                get;
                set;
            }
        }
    }
}
