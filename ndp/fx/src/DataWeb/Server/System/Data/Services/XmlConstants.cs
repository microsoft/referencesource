//---------------------------------------------------------------------
// <copyright file="XmlConstants.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      XmlConstants for metadata
// </summary>
//
// @owner  pratikp
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    /// <summary>
    /// Class that contains all the constants for various schemas.
    /// </summary>
    internal static class XmlConstants
    {
        #region CLR / Reflection constants.

        /// <summary>"InitializeService" method name for service initialize.</summary>
        internal const string ClrServiceInitializationMethodName = "InitializeService";

        #endregion CLR / Reflection constants.

        #region HTTP constants.

        /// <summary>id of the corresponding body</summary>
        internal const string HttpContentID = "Content-ID";

        /// <summary>byte-length of the corresponding body</summary>
        internal const string HttpContentLength = "Content-Length";

        /// <summary>mime-type of the corresponding body</summary>
        internal const string HttpContentType = "Content-Type";

        /// <summary>content disposition of the response (a hint how to handle the response)</summary>
        internal const string HttpContentDisposition = "Content-Disposition";

        /// <summary>'DataServiceVersion' - HTTP header name for data service version.</summary>
        internal const string HttpDataServiceVersion = "DataServiceVersion";

        /// <summary>'MaxDataServiceVersion' - HTTP header name for maximum understood data service version.</summary>
        internal const string HttpMaxDataServiceVersion = "MaxDataServiceVersion";

        /// <summary>'no-cache' - HTTP value for Cache-Control header.</summary>
        internal const string HttpCacheControlNoCache = "no-cache";

        /// <summary>'charset' - HTTP parameter name.</summary>
        internal const string HttpCharsetParameter = "charset";

        /// <summary>HTTP method name for GET requests.</summary>
        internal const string HttpMethodGet = "GET";

        /// <summary>HTTP method name for POST requests.</summary>
        internal const string HttpMethodPost = "POST";

        /// <summary> Http Put Method name - basically used for updating resource.</summary>
        internal const string HttpMethodPut = "PUT";

        /// <summary>HTTP method name for delete requests.</summary>
        internal const string HttpMethodDelete = "DELETE";

        /// <summary>HTTP method name for </summary>
        internal const string HttpMethodMerge = "MERGE";

        /// <summary>HTTP query string parameter value for expand.</summary>
        internal const string HttpQueryStringExpand = "$expand";

        /// <summary>HTTP query string parameter value for filtering.</summary>
        internal const string HttpQueryStringFilter = "$filter";

        /// <summary>HTTP query string parameter value for ordering.</summary>
        internal const string HttpQueryStringOrderBy = "$orderby";

        /// <summary>HTTP query string parameter value for skipping elements.</summary>
        internal const string HttpQueryStringSkip = "$skip";

        /// <summary>HTTP query string parameter value for limiting the number of elements.</summary>
        internal const string HttpQueryStringTop = "$top";

        /// <summary>HTTP query string parameter value for counting query result set</summary>
        internal const string HttpQueryStringInlineCount = "$inlinecount";

        /// <summary>HTTP query string parameter value for skipping results based on paging.</summary>
        internal const string HttpQueryStringSkipToken = "$skiptoken";

        /// <summary>Property prefix for the skip token property in expanded results for a skip token</summary>
        internal const string SkipTokenPropertyPrefix = "SkipTokenProperty";

        /// <summary>HTTP query string parameter value for counting query result set</summary>
        internal const string HttpQueryStringValueCount = "$count";

        /// <summary>HTTP query string parameter value for projection.</summary>
        internal const string HttpQueryStringSelect = "$select";

        /// <summary>'q' - HTTP q-value parameter name.</summary>
        internal const string HttpQValueParameter = "q";

        /// <summary>'X-HTTP-Method' - HTTP header name for requests that want to tunnel a method through POST.</summary>
        internal const string HttpXMethod = "X-HTTP-Method";

        /// <summary>HTTP name for Accept header</summary>
        internal const string HttpRequestAccept = "Accept";

        /// <summary>HTTP name for If-Match header</summary>
        internal const string HttpRequestAcceptCharset = "Accept-Charset";

        /// <summary>HTTP name for If-Match header</summary>
        internal const string HttpRequestIfMatch = "If-Match";

        /// <summary>HTTP name for If-None-Match header</summary>
        internal const string HttpRequestIfNoneMatch = "If-None-Match";

        /// <summary>multi-part keyword in content-type to identify batch separator</summary>
        internal const string HttpMultipartBoundary = "boundary";
#if ASTORIA_CLIENT
        /// <summary>multi-part mixed batch separator</summary>
        internal const string HttpMultipartBoundaryBatch = "batch";

        /// <summary>multi-part mixed changeset separator</summary>
        internal const string HttpMultipartBoundaryChangeSet = "changeset";
#endif

        /// <summary>'Allow' - HTTP response header for allowed verbs.</summary>
        internal const string HttpResponseAllow = "Allow";

        /// <summary>'no-cache' - HTTP value for Cache-Control header.</summary>
        internal const string HttpResponseCacheControl = "Cache-Control";

        /// <summary>HTTP name for ETag header</summary>
        internal const string HttpResponseETag = "ETag";

        /// <summary>HTTP name for location header</summary>
        internal const string HttpResponseLocation = "Location";

        /// <summary>HTTP name for Status-Code header</summary>
        internal const string HttpResponseStatusCode = "Status-Code";

        /// <summary>multi-part mixed batch separator for response stream</summary>
        internal const string HttpMultipartBoundaryBatchResponse = "batchresponse";

        /// <summary>multi-part mixed changeset separator</summary>
        internal const string HttpMultipartBoundaryChangesetResponse = "changesetresponse";

        /// <summary>Content-Transfer-Encoding header for batch requests.</summary>
        internal const string HttpContentTransferEncoding = "Content-Transfer-Encoding";

        /// <summary>Http Version in batching requests and response.</summary>
        internal const string HttpVersionInBatching = "HTTP/1.1";

        /// <summary>To checks if the resource exists or not.</summary>
        internal const string HttpAnyETag = "*";

        /// <summary>Weak etags in HTTP must start with W/.
        /// Look in http://www.ietf.org/rfc/rfc2616.txt?number=2616 section 14.19 for more information.</summary>
        internal const string HttpWeakETagPrefix = "W/\"";

        /// <summary>The character set the client wants the response to be in.</summary>
        internal const string HttpAcceptCharset = "Accept-Charset";

        /// <summary>The name of the Cookie HTTP header</summary>
        internal const string HttpCookie = "Cookie";

        /// <summary>The Slug header name. Used by ATOM to hint the server on which MR is being POSTed.</summary>
        internal const string HttpSlug = "Slug";

        #endregion HTTP constants.

        #region MIME constants.

        /// <summary>MIME type for requesting any media type.</summary>
        internal const string MimeAny = "*/*";

        /// <summary>MIME type for ATOM bodies (http://www.iana.org/assignments/media-types/application/).</summary>
        internal const string MimeApplicationAtom = "application/atom+xml";

        /// <summary>MIME type for ATOM Service Documents (http://tools.ietf.org/html/rfc5023#section-8).</summary>
        internal const string MimeApplicationAtomService = "application/atomsvc+xml";

        /// <summary>MIME type for JSON bodies (http://www.iana.org/assignments/media-types/application/).</summary>
        internal const string MimeApplicationJson = "application/json";

        /// <summary>MIME type general binary bodies (http://www.iana.org/assignments/media-types/application/).</summary>
        internal const string MimeApplicationOctetStream = "application/octet-stream";

        /// <summary>MIME type for batch requests - this mime type must be specified in CUD changesets or GET batch requests.</summary>
        internal const string MimeApplicationHttp = "application/http";

        /// <summary>'application' - MIME type for application types.</summary>
        internal const string MimeApplicationType = "application";

        /// <summary>MIME type for XML bodies.</summary>
        internal const string MimeApplicationXml = "application/xml";

        /// <summary>'json' - constant for MIME JSON subtypes.</summary>
        internal const string MimeJsonSubType = "json";

        /// <summary>"application/xml", MIME type for metadata requests.</summary>
        internal const string MimeMetadata = MimeApplicationXml;

        /// <summary>MIME type for changeset multipart/mixed</summary>
        internal const string MimeMultiPartMixed = "multipart/mixed";

        /// <summary>MIME type for plain text bodies.</summary>
        internal const string MimeTextPlain = "text/plain";

        /// <summary>'text' - MIME type for text subtypes.</summary>
        internal const string MimeTextType = "text";

        /// <summary>MIME type for XML bodies (deprecated).</summary>
        internal const string MimeTextXml = "text/xml";

        /// <summary>'xml' - constant for MIME xml subtypes.</summary>
        internal const string MimeXmlSubType = "xml";

        /// <summary>Content-Transfer-Encoding value for batch requests.</summary>
        internal const string BatchRequestContentTransferEncoding = "binary";

#if ASTORIA_CLIENT
        /// <summary>Link referring to a collection i.e. feed.</summary>
        internal const string LinkMimeTypeFeed = "application/atom+xml;type=feed";

        /// <summary>Link referring to an entity i.e. entry.</summary>
        internal const string LinkMimeTypeEntry = "application/atom+xml;type=entry";

        /// <summary>text for the utf8 encoding</summary>
        internal const string Utf8Encoding = "UTF-8";

        /// <summary>Default encoding used for writing textual media link entries</summary>
        internal const string MimeTypeUtf8Encoding = ";charset=" + Utf8Encoding;
#endif
        #endregion MIME constants.

        #region URI constants.

        /// <summary>A prefix that turns an absolute-path URI into an absolute-URI.</summary>
        internal const string UriHttpAbsolutePrefix = "http://host";

        /// <summary>A segment name in a URI that indicates metadata is being requested.</summary>
        internal const string UriMetadataSegment = "$metadata";

        /// <summary>A segment name in a URI that indicates a plain primitive value is being requested.</summary>
        internal const string UriValueSegment = "$value";

        /// <summary>A segment name in a URI that indicates metadata is being requested.</summary>
        internal const string UriBatchSegment = "$batch";

        /// <summary>A segment name in a URI that indicates that this is a link operation.</summary>
        internal const string UriLinkSegment = "$links";

        /// <summary>A segment name in a URI that indicates that this is a count operation.</summary>
        internal const string UriCountSegment = "$count";

        /// <summary>A const value for the query parameter $inlinecount to set counting mode to inline</summary>
        internal const string UriRowCountAllOption = "allpages";

        /// <summary>A const value for the query parameter $inlinecount to set counting mode to none</summary>
        internal const string UriRowCountOffOption = "none";

        #endregion URI constants.

        #region WCF constants.

        /// <summary>"Binary" - WCF element name for binary content in XML-wrapping streams.</summary>
        internal const string WcfBinaryElementName = "Binary";

        #endregion WCF constants.

        #region ATOM constants
        /// <summary>XML element name to mark content element in Atom.</summary>
        internal const string AtomContentElementName = "content";

        /// <summary>XML element name to mark entry element in Atom.</summary>
        internal const string AtomEntryElementName = "entry";

        /// <summary>XML element name to mark feed element in Atom.</summary>
        internal const string AtomFeedElementName = "feed";

#if ASTORIA_CLIENT
        /// <summary>'author' - XML element name for ATOM 'author' element for entries.</summary>
        internal const string AtomAuthorElementName = "author";

        /// <summary>'contributor' - XML element name for ATOM 'author' element for entries.</summary>
        internal const string AtomContributorElementName = "contributor";

        /// <summary>'category' - XML element name for ATOM 'category' element for entries.</summary>
        internal const string AtomCategoryElementName = "category";

        /// <summary>'scheme' - XML attribute name for ATOM 'scheme' attribute for categories.</summary>
        internal const string AtomCategorySchemeAttributeName = "scheme";

        /// <summary>'term' - XML attribute name for ATOM 'term' attribute for categories.</summary>
        internal const string AtomCategoryTermAttributeName = "term";

        /// <summary>XML element name to mark id element in Atom.</summary>
        internal const string AtomIdElementName = "id";

        /// <summary>XML element name to mark link element in Atom.</summary>
        internal const string AtomLinkElementName = "link";

        /// <summary>XML element name to mark link relation attribute in Atom.</summary>
        internal const string AtomLinkRelationAttributeName = "rel";

        /// <summary>Atom attribute that indicates the actual location for an entry's content.</summary>
        internal const string AtomContentSrcAttributeName = "src";

        /// <summary>XML element string for "next" links: [atom:link rel="next"]</summary>
        internal const string AtomLinkNextAttributeString = "next";

#endif
        /// <summary>Type of content for syndication property which can be one of Plaintext, Html or XHtml</summary>
        internal const string MetadataAttributeEpmContentKind = "FC_ContentKind";

        /// <summary>Whether to keep the property value in the content section</summary>
        internal const string MetadataAttributeEpmKeepInContent = "FC_KeepInContent";

        /// <summary>TargetNamespace prefix for non-syndication mapping</summary>
        internal const string MetadataAttributeEpmNsPrefix = "FC_NsPrefix";

        /// <summary>TargetNamespace URI for non-syndication mapping</summary>
        internal const string MetadataAttributeEpmNsUri = "FC_NsUri";

        /// <summary>Target element or attribute name</summary>
        internal const string MetadataAttributeEpmTargetPath = "FC_TargetPath";

        /// <summary>Source property name</summary>
        internal const string MetadataAttributeEpmSourcePath = "FC_SourcePath";

        /// <summary>author/email</summary>
        internal const string SyndAuthorEmail = "SyndicationAuthorEmail";

        /// <summary>author/name</summary>
        internal const string SyndAuthorName = "SyndicationAuthorName";

        /// <summary>author/uri</summary>
        internal const string SyndAuthorUri = "SyndicationAuthorUri";

        /// <summary>published</summary>
        internal const string SyndPublished = "SyndicationPublished";

        /// <summary>rights</summary>
        internal const string SyndRights = "SyndicationRights";

        /// <summary>summary</summary>
        internal const string SyndSummary = "SyndicationSummary";

        /// <summary>title</summary>
        internal const string SyndTitle = "SyndicationTitle";

        /// <summary>'updated' - XML element name for ATOM 'updated' element for entries.</summary>
        internal const string AtomUpdatedElementName = "updated";

        /// <summary>contributor/email</summary>
        internal const string SyndContributorEmail = "SyndicationContributorEmail";

        /// <summary>contributor/name</summary>
        internal const string SyndContributorName = "SyndicationContributorName";

        /// <summary>contributor/uri</summary>
        internal const string SyndContributorUri = "SyndicationContributorUri";

        /// <summary>updated</summary>
        internal const string SyndUpdated = "SyndicationUpdated";

        /// <summary>Plaintext</summary>
        internal const string SyndContentKindPlaintext = "text";

        /// <summary>HTML</summary>
        internal const string SyndContentKindHtml = "html";

        /// <summary>XHTML</summary>
        internal const string SyndContentKindXHtml = "xhtml";

        /// <summary>XML element name to mark href attribute element in Atom.</summary>
        internal const string AtomHRefAttributeName = "href";

        /// <summary>XML element name to mark summary element in Atom.</summary>
        internal const string AtomSummaryElementName = "summary";

        /// <summary>XML element name to mark author/name or contributor/name element in Atom.</summary>
        internal const string AtomNameElementName = "name";

        /// <summary>XML element name to mark author/email or contributor/email element in Atom.</summary>
        internal const string AtomEmailElementName = "email";

        /// <summary>XML element name to mark author/uri or contributor/uri element in Atom.</summary>
        internal const string AtomUriElementName = "uri";

        /// <summary>XML element name to mark published element in Atom.</summary>
        internal const string AtomPublishedElementName = "published";

        /// <summary>XML element name to mark rights element in Atom.</summary>
        internal const string AtomRightsElementName = "rights";

        /// <summary>XML element name to mark 'collection' element in APP.</summary>
        internal const string AtomPublishingCollectionElementName = "collection";

        /// <summary>XML element name to mark 'service' element in APP.</summary>
        internal const string AtomPublishingServiceElementName = "service";

        /// <summary>XML value for a default workspace in APP.</summary>
        internal const string AtomPublishingWorkspaceDefaultValue = "Default";

        /// <summary>XML element name to mark 'workspace' element in APP.</summary>
        internal const string AtomPublishingWorkspaceElementName = "workspace";

        /// <summary>XML element name to mark title element in Atom.</summary>
        internal const string AtomTitleElementName = "title";

        /// <summary>XML element name to mark title element in Atom.</summary>
        internal const string AtomTypeAttributeName = "type";

        /// <summary> Atom link relation attribute value for self links.</summary>
        internal const string AtomSelfRelationAttributeValue = "self";

        /// <summary> Atom link relation attribute value for edit links.</summary>
        internal const string AtomEditRelationAttributeValue = "edit";

        /// <summary> Atom link relation attribute value for edit-media links.</summary>
        internal const string AtomEditMediaRelationAttributeValue = "edit-media";

        /// <summary> Atom attribute which indicates the null value for the element.</summary>
        internal const string AtomNullAttributeName = "null";

        /// <summary> Atom attribute which indicates the etag value for the declaring entry element.</summary>
        internal const string AtomETagAttributeName = "etag";
        
        /// <summary>'Inline' - wrapping element for inlined entry/feed content.</summary>
        internal const string AtomInlineElementName = "inline";

        /// <summary>Element containing property values when 'content' is used for media link entries</summary>
        internal const string AtomPropertiesElementName = "properties";

        /// <summary>'count' element</summary>
        internal const string RowCountElement = "count";

        #endregion ATOM constants

        #region XML constants.

        /// <summary>'element', the XML element name for items in enumerations.</summary>
        internal const string XmlCollectionItemElementName = "element";

        /// <summary>XML element name for an error.</summary>
        internal const string XmlErrorElementName = "error";
        
        /// <summary>XML element name for an error code.</summary>
        internal const string XmlErrorCodeElementName = "code";

        /// <summary>XML element name for the inner error details.</summary>
        internal const string XmlErrorInnerElementName = "innererror";

        /// <summary>XML element name for an internal exception.</summary>
        internal const string XmlErrorInternalExceptionElementName = "internalexception";

        /// <summary>XML element name for an exception type.</summary>
        internal const string XmlErrorTypeElementName = "type";

        /// <summary>XML element name for an exception stack trace.</summary>
        internal const string XmlErrorStackTraceElementName = "stacktrace";
        
        /// <summary>XML element name for an error message.</summary>
        internal const string XmlErrorMessageElementName = "message";
        
        /// <summary>'false' literal, as used in XML.</summary>
        internal const string XmlFalseLiteral = "false";

        /// <summary>'true' literal, as used in XML.</summary>
        internal const string XmlTrueLiteral = "true";

        /// <summary>'INF' literal, as used in XML for infinity.</summary>
        internal const string XmlInfinityLiteral = "INF";

        /// <summary>'NaN' literal, as used in XML for not-a-number values.</summary>
        internal const string XmlNaNLiteral = "NaN";

        /// <summary>XML attribute value to indicate the base URI for a document or element.</summary>
        internal const string XmlBaseAttributeName = "base";

        /// <summary>'lang' XML attribute name for annotation language.</summary>
        internal const string XmlLangAttributeName = "lang";

        /// <summary>XML attribute name for whitespace parsing control.</summary>
        internal const string XmlSpaceAttributeName = "space";

        /// <summary>XML attribute value to indicate whitespace should be preserved.</summary>
        internal const string XmlSpacePreserveValue = "preserve";

        /// <summary>XML attribute name to pass to the XMLReader.GetValue API to get the xml:base attribute value.</summary>
        internal const string XmlBaseAttributeNameWithPrefix = "xml:base";

        #endregion XML constants.

        #region XML namespaces.

        /// <summary> Schema Namespace For Edm.</summary>
        internal const string EdmV1Namespace = "http://schemas.microsoft.com/ado/2006/04/edm";

        /// <summary> Schema Namespace For Edm 1.1.</summary>
        internal const string EdmV1dot1Namespace = "http://schemas.microsoft.com/ado/2007/05/edm";

        /// <summary> Schema Namespace For Edm 1.2.</summary>
        internal const string EdmV1dot2Namespace = "http://schemas.microsoft.com/ado/2008/01/edm";

        /// <summary> Schema Namespace For Edm 2.0.</summary>
        internal const string EdmV2Namespace = "http://schemas.microsoft.com/ado/2008/09/edm";
        
        /// <summary>XML namespace for data services.</summary>
        internal const string DataWebNamespace = "http://schemas.microsoft.com/ado/2007/08/dataservices";

        /// <summary>XML namespace for data service annotations.</summary>
        internal const string DataWebMetadataNamespace = "http://schemas.microsoft.com/ado/2007/08/dataservices/metadata";

        /// <summary>XML namespace for data service links.</summary>
        internal const string DataWebRelatedNamespace = "http://schemas.microsoft.com/ado/2007/08/dataservices/related/";

        /// <summary>ATOM Scheme Namespace For DataWeb.</summary>
        internal const string DataWebSchemeNamespace = "http://schemas.microsoft.com/ado/2007/08/dataservices/scheme";

        /// <summary>Schema Namespace for Atom Publishing Protocol.</summary>
        internal const string AppNamespace = "http://www.w3.org/2007/app";

        /// <summary> Schema Namespace For Atom.</summary>
        internal const string AtomNamespace = "http://www.w3.org/2005/Atom";

        /// <summary> Schema Namespace prefix For xmlns.</summary>
        internal const string XmlnsNamespacePrefix = "xmlns";

        /// <summary> Schema Namespace prefix For xml.</summary>
        internal const string XmlNamespacePrefix = "xml";

        /// <summary> Schema Namespace Prefix For DataWeb.</summary>
        internal const string DataWebNamespacePrefix = "d";

        /// <summary>'adsm' - namespace prefix for DataWebMetadataNamespace.</summary>
        internal const string DataWebMetadataNamespacePrefix = "m";

        /// <summary>'http://www.w3.org/2000/xmlns/' - namespace for namespace declarations.</summary>
        internal const string XmlNamespacesNamespace = "http://www.w3.org/2000/xmlns/";

        /// <summary> Edmx namespace in metadata document.</summary>
        internal const string EdmxNamespace = "http://schemas.microsoft.com/ado/2007/06/edmx";

        /// <summary> Prefix for Edmx Namespace in metadata document.</summary>
        internal const string EdmxNamespacePrefix = "edmx";

        #endregion XML namespaces.

        #region CDM Schema Xml NodeNames

        #region Constant node names in the CDM schema xml

        /// <summary> Association Element Name in csdl.</summary>
        internal const string Association = "Association";

        /// <summary> AssociationSet Element Name in csdl.</summary>
        internal const string AssociationSet = "AssociationSet";

        /// <summary> ComplexType Element Name in csdl.</summary>
        internal const string ComplexType = "ComplexType";

        /// <summary> Dependent Element Name in csdl.</summary>
        internal const string Dependent = "Dependent";

        /// <summary>Format string to describe a collection of a given type.</summary>
        internal const string EdmCollectionTypeFormat = "Collection({0})";

        /// <summary>EntitySet attribute name in CSDL documents.</summary>
        internal const string EdmEntitySetAttributeName = "EntitySet";

        /// <summary>FunctionImport element name in CSDL documents.</summary>
        internal const string EdmFunctionImportElementName = "FunctionImport";

        /// <summary>Mode attribute name in CSDL documents.</summary>
        internal const string EdmModeAttributeName = "Mode";

        /// <summary>Mode attribute value for 'in' direction in CSDL documents.</summary>
        internal const string EdmModeInValue = "In";

        /// <summary>Parameter element name in CSDL documents.</summary>
        internal const string EdmParameterElementName = "Parameter";

        /// <summary>ReturnType attribute name in CSDL documents.</summary>
        internal const string EdmReturnTypeAttributeName = "ReturnType";

        /// <summary> End Element Name in csdl.</summary>
        internal const string End = "End";

        /// <summary> EntityType Element Name in csdl.</summary>
        internal const string EntityType = "EntityType";

        /// <summary> EntityContainer Element Name in csdl.</summary>
        internal const string EntityContainer = "EntityContainer";

        /// <summary> Key Element Name in csdl.</summary>
        internal const string Key = "Key";

        /// <summary> NavigationProperty Element Name in csdl.</summary>
        internal const string NavigationProperty = "NavigationProperty";

        /// <summary> OnDelete Element Name in csdl.</summary>
        internal const string OnDelete = "OnDelete";

        /// <summary> Principal Element Name in csdl.</summary>
        internal const string Principal = "Principal";

        /// <summary> Property Element Name in csdl.</summary>
        internal const string Property = "Property";

        /// <summary> PropetyRef Element Name in csdl.</summary>
        internal const string PropertyRef = "PropertyRef";

        /// <summary> ReferentialConstraint Element Name in csdl.</summary>
        internal const string ReferentialConstraint = "ReferentialConstraint";

        /// <summary> Role Element Name in csdl.</summary>
        internal const string Role = "Role";

        /// <summary> Schema Element Name in csdl.</summary>
        internal const string Schema = "Schema";

        /// <summary> Edmx Element Name in the metadata document.</summary>
        internal const string EdmxElement = "Edmx";

        /// <summary> Edmx DataServices Element Name in the metadata document.</summary>
        internal const string EdmxDataServicesElement = "DataServices";

        /// <summary>Version attribute for the root Edmx Element in the metadata document.</summary>
        internal const string EdmxVersion = "Version";

        /// <summary>Value of the version attribute in the root edmx element in metadata document.</summary>
        internal const string EdmxVersionValue = "1.0";

        #endregion //Constant node names in the CDM schema xml

        #region const attribute names in the CDM schema XML

        /// <summary> Action attribute Name in csdl.</summary>
        internal const string Action = "Action";

        /// <summary> BaseType attribute Name in csdl.</summary>
        internal const string BaseType = "BaseType";

        /// <summary> EntitySet attribute and Element Name in csdl.</summary>
        internal const string EntitySet = "EntitySet";

        /// <summary> FromRole attribute Name in csdl.</summary>
        internal const string FromRole = "FromRole";

        /// <summary>Abstract attribute Name in csdl.</summary>
        internal const string Abstract = "Abstract";

        /// <summary>Multiplicity attribute Name in csdl.</summary>
        internal const string Multiplicity = "Multiplicity";

        /// <summary>Name attribute Name in csdl.</summary>
        internal const string Name = "Name";

        /// <summary>Namespace attribute Element Name in csdl.</summary>
        internal const string Namespace = "Namespace";

        /// <summary>ToRole attribute Name in csdl.</summary>
        internal const string ToRole = "ToRole";

        /// <summary>Type attribute Name in csdl.</summary>
        internal const string Type = "Type";

        /// <summary>Relationship attribute Name in csdl.</summary>
        internal const string Relationship = "Relationship";
        #endregion //const attribute names in the CDM schema XML

        #region values for multiplicity in Edm

        /// <summary>Value for Many multiplicity in csdl.</summary>
        internal const string Many = "*";

        /// <summary>Value for One multiplicity in csdl.</summary>
        internal const string One = "1";

        /// <summary>Value for ZeroOrOne multiplicity in csdl.</summary>
        internal const string ZeroOrOne = "0..1";
        #endregion

        #region Edm Facets Names and Values

        /// <summary>Nullable Facet Name in csdl.</summary>
        internal const string Nullable = "Nullable";

        /// <summary>Name of the concurrency attribute.</summary>
        internal const string ConcurrencyAttribute = "ConcurrencyMode";

        /// <summary>Value of the concurrency attribute.</summary>
        internal const string ConcurrencyFixedValue = "Fixed";

        #endregion

        #endregion // CDM Schema Xml NodeNames

        #region DataWeb Elements and Attributes.

        /// <summary>'MimeType' - attribute name for property MIME type attributes.</summary>
        internal const string DataWebMimeTypeAttributeName = "MimeType";

        /// <summary>'OpenType' - attribute name to indicate a type is an OpenType property.</summary>
        internal const string DataWebOpenTypeAttributeName = "OpenType";

        /// <summary>'HasStream' - attribute name to indicate a type has a default stream property.</summary>
        internal const string DataWebAccessHasStreamAttribute = "HasStream";

        /// <summary>'true' - attribute value to indicate a type has a default stream property.</summary>
        internal const string DataWebAccessDefaultStreamPropertyValue = "true";

        /// <summary>Attribute to indicate whether this is a default entity container or not.</summary>
        internal const string IsDefaultEntityContainerAttribute = "IsDefaultEntityContainer";

        /// <summary>Attribute name in the csdl to indicate whether the service operation must be called using POST or GET verb.</summary>
        internal const string ServiceOperationHttpMethodName = "HttpMethod";

        /// <summary>uri element name for link bind/unbind operations</summary>
        internal const string UriElementName = "uri";

        /// <summary>next element name for link paging</summary>
        internal const string NextElementName = "next";

        /// <summary>XML element name for writing out collection of links.</summary>
        internal const string LinkCollectionElementName = "links";

        #endregion DataWeb Elements and Attributes.

        #region JSON Format constants

        /// <summary>JSON property name for an error.</summary>
        internal const string JsonError = "error";

        /// <summary>JSON property name for an error code.</summary>
        internal const string JsonErrorCode = "code";

        /// <summary>JSON property name for the inner error details.</summary>
        internal const string JsonErrorInner = "innererror";

        /// <summary>JSON property name for an internal exception.</summary>
        internal const string JsonErrorInternalException = "internalexception";

        /// <summary>JSON property name for an error message.</summary>
        internal const string JsonErrorMessage = "message";

        /// <summary>JSON property name for an exception stack trace.</summary>
        internal const string JsonErrorStackTrace = "stacktrace";

        /// <summary>JSON property name for an exception type.</summary>
        internal const string JsonErrorType = "type";

        /// <summary>JSON property name for an error message value.</summary>
        internal const string JsonErrorValue = "value";

        /// <summary>metadata element name in json payload.</summary>
        internal const string JsonMetadataString = "__metadata";

        /// <summary>uri element name in json payload.</summary>
        internal const string JsonUriString = "uri";

        /// <summary>type element name in json payload.</summary>
        internal const string JsonTypeString = "type";

        /// <summary>edit_media element name in json payload.</summary>
        internal const string JsonEditMediaString = "edit_media";

        /// <summary>media_src element name in json payload.</summary>
        internal const string JsonMediaSrcString = "media_src";

        /// <summary>content_type element name in json payload.</summary>
        internal const string JsonContentTypeString = "content_type";

        /// <summary>media_etag element name in json payload.</summary>
        internal const string JsonMediaETagString = "media_etag";

        /// <summary>deferred element name in json payload.</summary>
        internal const string JsonDeferredString = "__deferred";

        /// <summary>etag element name in json payload.</summary>
        internal const string JsonETagString = "etag";

        /// <summary>row count element name in json payload</summary>
        internal const string JsonRowCountString = "__count";

        /// <summary>next page link element name in json payload</summary>
        internal const string JsonNextString = "__next";
        
        #endregion //JSON Format constants

        #region Edm Primitive Type Names
        /// <summary>namespace for edm primitive types.</summary>
        internal const string EdmNamespace = "Edm";

        /// <summary>edm binary primitive type name</summary>
        internal const string EdmBinaryTypeName = "Edm.Binary";

        /// <summary>edm boolean primitive type name</summary>
        internal const string EdmBooleanTypeName = "Edm.Boolean";

        /// <summary>edm byte primitive type name</summary>
        internal const string EdmByteTypeName = "Edm.Byte";

        /// <summary>edm datetime primitive type name</summary>
        internal const string EdmDateTimeTypeName = "Edm.DateTime";

        /// <summary>edm decimal primitive type name</summary>
        internal const string EdmDecimalTypeName = "Edm.Decimal";

        /// <summary>edm double primitive type name</summary>
        internal const string EdmDoubleTypeName = "Edm.Double";

        /// <summary>edm guid primitive type name</summary>
        internal const string EdmGuidTypeName = "Edm.Guid";

        /// <summary>edm single primitive type name</summary>
        internal const string EdmSingleTypeName = "Edm.Single";

        /// <summary>edm sbyte primitive type name</summary>
        internal const string EdmSByteTypeName = "Edm.SByte";

        /// <summary>edm int16 primitive type name</summary>
        internal const string EdmInt16TypeName = "Edm.Int16";

        /// <summary>edm int32 primitive type name</summary>
        internal const string EdmInt32TypeName = "Edm.Int32";

        /// <summary>edm int64 primitive type name</summary>
        internal const string EdmInt64TypeName = "Edm.Int64";

        /// <summary>edm string primitive type name</summary>
        internal const string EdmStringTypeName = "Edm.String";
        #endregion

        #region Astoria Constants

        /// <summary>'1.0' - the version 1.0 text for a data service.</summary>
        internal const string DataServiceVersion1Dot0 = "1.0";

        /// <summary>'2.0' - the version 2.0 text for a data service.</summary>
        internal const string DataServiceVersion2Dot0 = "2.0";

        /// <summary>'2.0;' - the text for the current server version text.</summary>
        internal const string DataServiceVersionCurrent = DataServiceVersion2Dot0 + ";";

        /// <summary>1 - the version 1 text for a data service.</summary>
        internal const int DataServiceVersionCurrentMajor = 1;

        /// <summary>0 - the current minor version for a data service.</summary>
        internal const int DataServiceVersionCurrentMinor = 0;

        /// <summary>'binary' constant prefixed to binary literals.</summary>
        internal const string LiteralPrefixBinary = "binary";

        /// <summary>'datetime' constant prefixed to datetime literals.</summary>
        internal const string LiteralPrefixDateTime = "datetime";

        /// <summary>'guid' constant prefixed to guid literals.</summary>
        internal const string LiteralPrefixGuid = "guid";

        /// <summary>'X': Prefix to binary type string representation.</summary>
        internal const string XmlBinaryPrefix = "X";

        /// <summary>'M': Suffix for decimal type's string representation</summary>
        internal const string XmlDecimalLiteralSuffix = "M";

        /// <summary>'L': Suffix for long (int64) type's string representation</summary>
        internal const string XmlInt64LiteralSuffix = "L";

        /// <summary>'f': Suffix for float (single) type's string representation</summary>
        internal const string XmlSingleLiteralSuffix = "f";

        /// <summary>'D': Suffix for double (Real) type's string representation</summary>
        internal const string XmlDoubleLiteralSuffix = "D";

        /// <summary>null liternal that needs to be return in ETag value when the value is null</summary>
        internal const string NullLiteralInETag = "null";

        /// <summary>Incoming message property name for the original reqeust uri</summary>
        internal const string MicrosoftDataServicesRequestUri = "MicrosoftDataServicesRequestUri";

        /// <summary>Incoming message property name for the original root service uri</summary>
        internal const string MicrosoftDataServicesRootUri = "MicrosoftDataServicesRootUri";

        #endregion // Astoria Constants

        #region EF constants

        /// <summary>Full name for the StoreGeneratedPattern attribute in csdl</summary>
        internal const string StoreGeneratedPattern = "http://schemas.microsoft.com/ado/2006/04/edm/ssdl:StoreGeneratedPattern";
        #endregion //EF constants
    }
}
