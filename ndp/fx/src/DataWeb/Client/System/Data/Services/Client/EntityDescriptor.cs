//---------------------------------------------------------------------
// <copyright file="EntityDescriptor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// represents the object and state
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Diagnostics;
    using System.Globalization;

    /// <summary>
    /// represents the cached entity
    /// </summary>
    [DebuggerDisplay("State = {state}, Uri = {editLink}, Element = {entity.GetType().ToString()}")]
    public sealed class EntityDescriptor : Descriptor
    {
        #region Fields

        /// <summary>uri to identitfy the entity</summary>
        /// <remarks>&lt;atom:id&gt;identity&lt;/id&gt;</remarks>
        private String identity;

        /// <summary>entity</summary>
        private object entity;

        /// <summary>entity tag</summary>
        private string etag;

        /// <summary>ETag for the media resource if this is a media link entry.</summary>
        private string streamETag;

        /// <summary>The parent resource that this resource is linked to.</summary>
        private EntityDescriptor parentDescriptor;

        /// <summary>Parent resource property to which this node is linked</summary>
        private string parentProperty;

        /// <summary>String storing the server type related to this entity</summary>
        private string serverTypeName;

        /// <summary>uri to query the entity</summary>
        /// <remarks>&lt;atom:link rel="self" href="queryLink" /&gt;</remarks>
        private Uri selfLink;

        /// <summary>uri to edit the entity. In case of deep add, this can also refer to the navigation property name.</summary>
        /// <remarks>&lt;atom:link rel="edit" href="editLink" /&gt;</remarks>
        private Uri editLink;

        /// <summary>In case of Media Link Entry, this is the link to the Media Resource (content).
        /// Otherwise this is null.</summary>
        /// <remarks>&lt;atom:content src="contentLink" /&gt;</remarks>
        private Uri readStreamLink;

        /// <summary>In case of Media Link Entry, this is the edit-media link value.
        /// Otherwise this is null.</summary>
        /// <remarks>&lt;atom:link rel="edit-media" href="editMediaLink" /&gt;</remarks>
        private Uri editMediaLink;

        /// <summary>The save stream associated with this MLE.</summary>
        /// <remarks>When set the entity is treated as MLE and will participate in SaveChanges 
        /// (even if its state is Unmodified). If the state of the entity is Added first
        /// the MR will be POSTed using this stream as the content then the entity will be PUT/MERGE as MLE.
        /// If the state of the entity is Unmodified only the MR will be PUT using this stream as the content.
        /// If the state of the entity is Changed first the MR will be PUT using this stream as the content
        /// and then the entity will be PUT/MERGE as MLE.
        /// If the state of the entity is Deleted this stream is not going to be used.
        /// 
        /// SaveChanges will call CloseSaveStream on this object after it returns regardless of it succeeding or not.
        /// SaveChanges will call the method even if it didn't get to process the entity (failed sooner).
        /// The CloseSaveStream method will close the stream if it's owned by the object and will set this field to null.</remarks>
        private DataServiceContext.DataServiceSaveStream saveStream;

        /// <summary>Set to true if the entity represented by this box is an MLE entity.</summary>
        /// <remarks>Since in some cases we don't know upfront if the entity is MLE or not, we use this boolean to remember the fact
        /// once we learn it. For example for POCO entities the entity becomes MLE only once a SetSaveStream is called on it.
        /// Also once the entity has become MLE it can't change that while we're tracking it (since it will be MLE on the server as well).
        /// That's why we only set thhis to true and never back to false.</remarks>
        private bool mediaLinkEntry;

        /// <summary>
        /// Describes whether the SaveStream is for Insert or Update.
        /// The value NoStream is for both non-MLEs and MLEs with unmodified stream.
        /// </summary>
        private StreamStates streamState;

        /// <summary>
        /// The entity set name to be used to insert the entity.
        /// </summary>
        private string entitySetName;

        #endregion

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="identity">resource Uri</param>
        /// <param name="selfLink">query link of entity</param>
        /// <param name="editLink">edit link of entity</param>
        /// <param name="entity">entity</param>
        /// <param name="parentEntity">parent entity</param>
        /// <param name="parentProperty">Parent entity property to which this entity is linked</param>
        /// <param name="entitySetName">name of the entity set to which the entity belongs.</param>
        /// <param name="etag">etag</param>
        /// <param name="state">entity state</param>
        internal EntityDescriptor(String identity, Uri selfLink, Uri editLink, object entity, EntityDescriptor parentEntity, string parentProperty, string entitySetName, string etag, EntityStates state)
            : base(state)
        {
            Debug.Assert(entity != null, "entity is null");
            Debug.Assert((parentEntity == null && parentProperty == null) || (parentEntity != null && parentProperty != null), "When parentEntity is specified, must also specify parentProperyName");

#if DEBUG
            if (state == EntityStates.Added)
            {
                Debug.Assert(identity == null && selfLink == null && editLink == null && etag == null, "For objects in added state, identity, self-link, edit-link and etag must be null");
                Debug.Assert((!String.IsNullOrEmpty(entitySetName) && parentEntity == null && String.IsNullOrEmpty(parentProperty)) ||
                             (String.IsNullOrEmpty(entitySetName) && parentEntity != null && !String.IsNullOrEmpty(parentProperty)),
                             "For entities in added state, entity set name or the insert path must be specified");
            }
            else
            {
                // For some read-only service, editlink can be null. They can only specify self-links.
                Debug.Assert(identity != null, "For objects in non-added state, identity must never be null");
                Debug.Assert(String.IsNullOrEmpty(entitySetName) && String.IsNullOrEmpty(parentProperty) && parentEntity == null, "For non-added entities, the entity set name and the insert path must be null");
            }
#endif

            // Identity can be null if object is just added (AddObject)
            this.identity = identity;
            this.selfLink = selfLink;
            this.editLink = editLink;

            this.parentDescriptor = parentEntity;
            this.parentProperty = parentProperty;
            this.entity = entity;
            this.etag = etag;
            this.entitySetName = entitySetName;
        }

        #region Properties

        /// <summary>entity uri identity</summary>
        public String Identity
        {
            get
            {
                return this.identity; 
            }

            internal set
            {
                Util.CheckArgumentNotEmpty(value, "Identity");
                this.identity = value;
                this.parentDescriptor = null;
                this.parentProperty = null;
                this.entitySetName = null;
            }
        }

        /// <summary>uri to query entity</summary>
        public Uri SelfLink
        {
            get { return this.selfLink; }
            internal set { this.selfLink = value; }
        }

        /// <summary>uri to edit entity</summary>
        public Uri EditLink
        {
            get { return this.editLink; }
            internal set { this.editLink = value; }
        }

        /// <summary>
        /// If the entity for the box is an MLE this property stores the content source URI of the MLE.
        /// That is, it stores the read URI for the associated MR.
        /// Setting it to non-null marks the entity as MLE.
        /// </summary>
        public Uri ReadStreamUri
        {
            get
            {
                return this.readStreamLink;
            }

            internal set
            {
                this.readStreamLink = value;
                if (value != null)
                {
                    this.mediaLinkEntry = true;
                }
            }
        }

        /// <summary>
        /// If the entity for the box is an MLE this property stores the edit-media link URI.
        /// That is, it stores the URI to send PUTs for the associated MR.
        /// Setting it to non-null marks the entity as MLE.
        /// </summary>
        public Uri EditStreamUri
        {
            get
            {
                return this.editMediaLink;
            }

            internal set
            {
                this.editMediaLink = value;
                if (value != null)
                {
                    this.mediaLinkEntry = true;
                }
            }
        }

        /// <summary>entity</summary>
        public object Entity
        {
            get { return this.entity; }
        }

        /// <summary>etag</summary>
        public string ETag
        {
            get { return this.etag; }
            internal set { this.etag = value; }
        }

        /// <summary>ETag for the media resource if this is a media link entry.</summary>
        public string StreamETag
        {
            get 
            { 
                return this.streamETag; 
            }

            internal set 
            {
                Debug.Assert(this.mediaLinkEntry == true, "this.mediaLinkEntry == true");
                this.streamETag = value; 
            }
        }

        /// <summary>Parent entity descriptor.</summary>
        /// <remarks>This is only set for entities added through AddRelateObject call</remarks>
        public EntityDescriptor ParentForInsert
        {
            get { return this.parentDescriptor; }
        }

        /// <summary>Parent entity property to which this entity is linked</summary>
        public string ParentPropertyForInsert
        {
            get { return this.parentProperty; }
        }

        /// <summary>Server type string</summary>
        public String ServerTypeName
        {
            get { return this.serverTypeName; }
            internal set { this.serverTypeName = value; }
        }

        #endregion

        #region Internal Properties
        // These properties are used internally for state tracking
        
        /// <summary>Parent entity</summary>
        internal object ParentEntity
        {
            get { return this.parentDescriptor != null ? this.parentDescriptor.entity : null; }
        }

        /// <summary>this is a entity</summary>
        internal override bool IsResource
        {
            get { return true; }
        }

        /// <summary>
        /// Returns true if the resource was inserted via its parent. E.g. POST customer(0)/Orders
        /// </summary>
        internal bool IsDeepInsert
        {
            get { return this.parentDescriptor != null; }
        }

        /// <summary>
        /// The stream which contains the new content for the MR associated with this MLE.
        /// This stream is used during SaveChanges to POST/PUT the MR.
        /// Setting it to non-null marks the entity as MLE.
        /// </summary>
        internal DataServiceContext.DataServiceSaveStream SaveStream
        {
            get
            {
                return this.saveStream;
            }

            set
            {
                this.saveStream = value;
                if (value != null)
                {
                    this.mediaLinkEntry = true;
                }
            }
        }

        /// <summary>
        /// Describes whether the SaveStream is for Insert or Update.
        /// The value NoStream is for both non-MLEs and MLEs with unmodified stream.
        /// </summary>
        internal StreamStates StreamState
        {
            get
            {
                return this.streamState;
            }

            set
            {
                this.streamState = value;
                Debug.Assert(this.streamState == StreamStates.NoStream || this.mediaLinkEntry, "this.streamState == StreamStates.NoStream || this.mediaLinkEntry");
                Debug.Assert(
                    (this.saveStream == null && this.streamState == StreamStates.NoStream) || (this.saveStream != null && this.streamState != StreamStates.NoStream),
                    "(this.saveStream == null && this.streamState == StreamStates.NoStream) || (this.saveStream != null && this.streamState != StreamStates.NoStream)");
            }
        }

        /// <summary>
        /// Returns true if we know that the entity is MLE. Note that this does not include the information
        /// from the entity type. So if the entity was attributed with HasStream for example
        /// this boolean might not be aware of it.
        /// </summary>
        internal bool IsMediaLinkEntry
        {
            get { return this.mediaLinkEntry; }
        }
        
        /// <summary>
        /// Returns true if the entry has been modified (and thus should participate in SaveChanges).
        /// </summary>
        internal override bool IsModified
        {
            get
            {
                if (base.IsModified)
                {
                    return true;
                }
                else
                {
                    // If the entity is not modified but it does have a save stream associated with it
                    // it means that the MR for the MLE should be updated and thus we need to consider
                    // the entity as modified (so that it shows up during SaveChanges)
                    return this.saveStream != null;
                }
            }
        }
        
        #endregion

        #region Internal Methods

        /// <summary>uri to edit the entity</summary>
        /// <param name="baseUriWithSlash">baseUriWithSlash</param>
        /// <param name="queryLink">whether to return the query link or edit link</param>
        /// <returns>absolute uri which can be used to edit the entity</returns>
        internal Uri GetResourceUri(Uri baseUriWithSlash, bool queryLink)
        {
            // If the entity was inserted using the AddRelatedObject API
            if (this.parentDescriptor != null)
            {
                // This is the batch scenario, where the entity might not have been saved yet, and there is another operation 
                // (for e.g. PUT $1/links/BestFriend or something). Hence we need to generate a Uri with the changeorder number.
                if (this.parentDescriptor.Identity == null)
                {
                    return Util.CreateUri(
                        Util.CreateUri(baseUriWithSlash, new Uri("$" + this.parentDescriptor.ChangeOrder.ToString(CultureInfo.InvariantCulture), UriKind.Relative)),
                        Util.CreateUri(this.parentProperty, UriKind.Relative));
                }
                else
                {
                    return Util.CreateUri(Util.CreateUri(baseUriWithSlash, this.parentDescriptor.GetLink(queryLink)), this.GetLink(queryLink));
                }
            }
            else
            {
                return Util.CreateUri(baseUriWithSlash, this.GetLink(queryLink));
            }
        }

        /// <summary>is the entity the same as the source or target entity</summary>
        /// <param name="related">related end</param>
        /// <returns>true if same as source or target entity</returns>
        internal bool IsRelatedEntity(LinkDescriptor related)
        {
            return ((this.entity == related.Source) || (this.entity == related.Target));
        }

        /// <summary>
        /// Return the related end for this resource. One should call this method, only if the resource is inserted via deep resource.
        /// </summary>
        /// <returns>returns the related end via which the resource was inserted.</returns>
        internal LinkDescriptor GetRelatedEnd()
        {
            Debug.Assert(this.IsDeepInsert, "For related end, this must be a deep insert");
            Debug.Assert(this.Identity == null, "If the identity is set, it means that the edit link no longer has the property name");

            return new LinkDescriptor(this.parentDescriptor.entity, this.parentProperty, this.entity);
        }

        /// <summary>
        /// Closes the save stream if there's any and sets it to null
        /// </summary>
        internal void CloseSaveStream()
        {
            if (this.saveStream != null)
            {
                DataServiceContext.DataServiceSaveStream stream = this.saveStream;
                this.saveStream = null;
                stream.Close();
            }
        }

        /// <summary>
        /// Returns the absolute URI for the media resource associated with this entity
        /// </summary>
        /// <param name="serviceBaseUri">The base uri of the service.</param>
        /// <returns>Absolute URI of the media resource for this entity, or null if the entity is not an MLE.</returns>
        internal Uri GetMediaResourceUri(Uri serviceBaseUri)
        {
            return this.ReadStreamUri == null ? null : Util.CreateUri(serviceBaseUri, this.ReadStreamUri);
        }

        /// <summary>
        /// Returns the absolute URI for editing the media resource associated with this entity
        /// </summary>
        /// <param name="serviceBaseUri">The base uri of the service.</param>
        /// <returns>Absolute URI for editing the media resource for this entity, or null if the entity is not an MLE.</returns>
        internal Uri GetEditMediaResourceUri(Uri serviceBaseUri)
        {
            return this.EditStreamUri == null ? null : Util.CreateUri(serviceBaseUri, this.EditStreamUri);
        }

        /// <summary>
        /// In V1, we used to not support self links. Hence we used to use edit links as self links.
        /// IN V2, we are adding support for self links. But if there are not specified, we need to 
        /// fall back on the edit link.
        /// </summary>
        /// <param name="queryLink">whether to get query link or the edit link.</param>
        /// <returns>the query or the edit link, as specified in the <paramref name="queryLink"/> parameter.</returns>
        private Uri GetLink(bool queryLink)
        {
            // If asked for a self link and self-link is present, return self link
            if (queryLink && this.SelfLink != null)
            {
                return this.SelfLink;
            }

            // otherwise return edit link if present.
            if (this.EditLink != null)
            {
                return this.EditLink;
            }

            // If both self and edit links are not specified, the entity must be in added state, and we need
            // to compute the relative link from the entity set name or the parent property.
            Debug.Assert(this.State == EntityStates.Added, "the entity must be in added state");
            if (!String.IsNullOrEmpty(this.entitySetName))
            {
                return Util.CreateUri(this.entitySetName, UriKind.Relative);
            }
            else
            {
                return Util.CreateUri(this.parentProperty, UriKind.Relative);
            }
        }

        #endregion
    }
}
