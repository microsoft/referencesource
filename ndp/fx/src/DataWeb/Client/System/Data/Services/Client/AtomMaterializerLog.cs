//---------------------------------------------------------------------
// <copyright file="AtomMaterializerLog.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class that can keep a record of changes done to the
// data service state.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Linq;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to keep a log of changes done by the materializer.
    /// </summary>
    internal class AtomMaterializerLog
    {
        #region Private fields.

        /// <summary>Associated data context for the log.</summary>
        private readonly DataServiceContext context;

        /// <summary>Dictionary of identity URI to instances created during previous AppendOnly moves.</summary>
        private readonly Dictionary<String, AtomEntry> appendOnlyEntries;

        /// <summary>Dictionary of identity URI to entries with media that aren't otherwise tracked.</summary>
        private readonly Dictionary<String, AtomEntry> foundEntriesWithMedia;

        /// <summary>Dictionary of identity URI to tracked entities.</summary>
        private readonly Dictionary<String, AtomEntry> identityStack;

        /// <summary>List of link descriptors (data for links and state).</summary>
        private readonly List<LinkDescriptor> links;

        /// <summary>Merge option used to apply changes.</summary>
        private readonly MergeOption mergeOption;

        /// <summary>Target instance to refresh.</summary>
        private object insertRefreshObject;

        #endregion Private fields.

        #region Constructors.

        /// <summary>
        /// Initializes a new <see cref="AtomMaterializerLog"/> instance.
        /// </summary>
        /// <param name="context">Associated data context for the log.</param>
        /// <param name="mergeOption">Merge option used to apply changes.</param>
        /// <remarks>
        /// Note that the merge option can't be changed.
        /// </remarks>
        internal AtomMaterializerLog(DataServiceContext context, MergeOption mergeOption)
        {
            Debug.Assert(context != null, "context != null");
            this.appendOnlyEntries = new Dictionary<string, AtomEntry>(EqualityComparer<String>.Default);
            this.context = context;
            this.mergeOption = mergeOption;
            this.foundEntriesWithMedia = new Dictionary<String, AtomEntry>(EqualityComparer<String>.Default);
            this.identityStack = new Dictionary<String, AtomEntry>(EqualityComparer<String>.Default);
            this.links = new List<LinkDescriptor>();
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Whether changes are being tracked.</summary>
        internal bool Tracking
        {
            get 
            { 
                return this.mergeOption != MergeOption.NoTracking; 
            }
        }

        #endregion Internal properties.

        #region Internal methods.

        /// <summary>Applies all accumulated changes to the associated data context.</summary>
        /// <remarks>The log should be cleared after this method successfully executed.</remarks>
        internal void ApplyToContext()
        {
            Debug.Assert(
                this.mergeOption != MergeOption.OverwriteChanges || this.foundEntriesWithMedia.Count == 0,
                "mergeOption != MergeOption.OverwriteChanges || foundEntriesWithMedia.Count == 0 - we only use the 'entries-with-media' lookaside when we're not in overwrite mode, otherwise we track everything through identity stack");

            if (!this.Tracking)
            {
                return;
            }

            foreach (KeyValuePair<String, AtomEntry> entity in this.identityStack)
            {
                AtomEntry entry = entity.Value;
                if (entry.CreatedByMaterializer ||
                    entry.ResolvedObject == this.insertRefreshObject ||
                    entry.ShouldUpdateFromPayload)
                {
                    // Create a new descriptor and try to attach, if one already exists, get the existing reference instead.
                    EntityDescriptor descriptor = new EntityDescriptor(entity.Key, entry.QueryLink, entry.EditLink, entry.ResolvedObject, null, null, null, entry.ETagText, EntityStates.Unchanged);
                    descriptor = this.context.InternalAttachEntityDescriptor(descriptor, false);

                    // we should always reset descriptor's state to Unchanged (old v1 behaviour)
                    descriptor.State = EntityStates.Unchanged;

                    this.ApplyMediaEntryInformation(entry, descriptor);
                    descriptor.ServerTypeName = entry.TypeName;
                }
                else
                {
                    // Refresh the entity state indirectly by calling TryGetEntity.
                    EntityStates state;
                    this.context.TryGetEntity(entity.Key, entry.ETagText, this.mergeOption, out state);
                }
            }

            // Regardless of the merge mode, media link information should
            // always be applied to the context.
            foreach (AtomEntry entry in this.foundEntriesWithMedia.Values)
            {
                Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise it wasn't found");
                EntityDescriptor descriptor = this.context.GetEntityDescriptor(entry.ResolvedObject);
                this.ApplyMediaEntryInformation(entry, descriptor);
            }

            foreach (LinkDescriptor link in this.links)
            {
                if (EntityStates.Added == link.State)
                {
                    // Added implies collection
                    if ((EntityStates.Deleted == this.context.GetEntityDescriptor(link.Target).State) ||
                        (EntityStates.Deleted == this.context.GetEntityDescriptor(link.Source).State))
                    {
                        this.context.DeleteLink(link.Source, link.SourceProperty, link.Target);
                    }
                    else
                    {
                        this.context.AttachLink(link.Source, link.SourceProperty, link.Target, this.mergeOption);
                    }
                }
                else if (EntityStates.Modified == link.State)
                {
                    // Modified implies reference
                    object target = link.Target;
                    if (MergeOption.PreserveChanges == this.mergeOption)
                    {
                        LinkDescriptor end = this.context.GetLinks(link.Source, link.SourceProperty).FirstOrDefault();
                        if (null != end && null == end.Target)
                        {
                            // leave the SetLink(link.Source, link.SourceProperty, null)
                            continue;
                        }

                        if ((null != target) && (EntityStates.Deleted == this.context.GetEntityDescriptor(target).State) ||
                            (EntityStates.Deleted == this.context.GetEntityDescriptor(link.Source).State))
                        {
                            target = null;
                        }
                    }

                    this.context.AttachLink(link.Source, link.SourceProperty, target, this.mergeOption);
                }
                else
                {
                    // detach link
                    Debug.Assert(EntityStates.Detached == link.State, "not detached link");
                    this.context.DetachLink(link.Source, link.SourceProperty, link.Target);
                }
            }
        }

        /// <summary>Clears all state in the log.</summary>
        internal void Clear()
        {
            this.foundEntriesWithMedia.Clear();
            this.identityStack.Clear();
            this.links.Clear();
            this.insertRefreshObject = null;
        }

        /// <summary>
        /// Invoke this method to notify the log that an existing
        /// instance was found while resolving an object.
        /// </summary>
        /// <param name="entry">Entry for instance.</param>
        internal void FoundExistingInstance(AtomEntry entry)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(ShouldTrackWithContext(entry), "Existing entries should be entity");

            if (this.mergeOption == MergeOption.OverwriteChanges)
            {
                this.identityStack[entry.Identity] = entry;
            }
            else if (this.Tracking && entry.MediaLinkEntry == true)
            {
                this.foundEntriesWithMedia[entry.Identity] = entry;
            }
        }

        /// <summary>
        /// Invoke this method to notify the log that the
        /// target instance of a "directed" update was found.
        /// </summary>
        /// <param name="entry">Entry found.</param>
        /// <remarks>
        /// The target instance is typically the object that we
        /// expect will get refreshed by the response from a POST
        /// method.
        /// 
        /// For example if a create a Customer and POST it to
        /// a service, the response of the POST will return the
        /// re-serialized instance, with (important!) server generated
        /// values and URIs.
        /// </remarks>
        internal void FoundTargetInstance(AtomEntry entry)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise this is not a target");

            if (ShouldTrackWithContext(entry))
            {
                this.context.AttachIdentity(entry.Identity, entry.QueryLink, entry.EditLink, entry.ResolvedObject, entry.ETagText);
                this.identityStack.Add(entry.Identity, entry);
                this.insertRefreshObject = entry.ResolvedObject;
            }
        }

        /// <summary>Attempts to resolve an entry from those tracked in the log.</summary>
        /// <param name="entry">Entry to resolve.</param>
        /// <param name="existingEntry">
        /// After invocation, an existing entry with the same identity as 
        /// <paramref name="entry"/>; possibly null.
        /// </param>
        /// <returns>true if an existing entry was found; false otherwise.</returns>
        internal bool TryResolve(AtomEntry entry, out AtomEntry existingEntry)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.Identity != null, "entry.Identity != null");

            if (this.identityStack.TryGetValue(entry.Identity, out existingEntry))
            {
                return true;
            }

            if (this.appendOnlyEntries.TryGetValue(entry.Identity, out existingEntry))
            {
                // The AppendOnly entries are valid only as long as they were not modified
                // between calls to .MoveNext().
                EntityStates state;
                this.context.TryGetEntity(entry.Identity, entry.ETagText, this.mergeOption, out state);
                if (state == EntityStates.Unchanged)
                {
                    return true;
                }
                else
                {
                    this.appendOnlyEntries.Remove(entry.Identity);
                }
            }

            existingEntry = null;
            return false;
        }

        /// <summary>
        /// Invoke this method to notify the log that a new link was
        /// added to a collection.
        /// </summary>
        /// <param name="source">
        /// Instance with the collection to which <paramref name="target"/> 
        /// was added.
        /// </param>
        /// <param name="propertyName">Property name for collection.</param>
        /// <param name="target">Object which was added.</param>
        internal void AddedLink(AtomEntry source, string propertyName, object target)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(propertyName != null, "propertyName != null");

            if (!this.Tracking)
            {
                return;
            }

            if (ShouldTrackWithContext(source) && ShouldTrackWithContext(target))
            {
                LinkDescriptor item = new LinkDescriptor(source.ResolvedObject, propertyName, target, EntityStates.Added);
                this.links.Add(item);
            }
        }

        /// <summary>
        /// Invoke this method to notify the log that a new instance
        /// was created.
        /// </summary>
        /// <param name="entry">Entry for the created instance.</param>
        internal void CreatedInstance(AtomEntry entry)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(entry.ResolvedObject != null, "entry.ResolvedObject != null -- otherwise, what did we create?");
            Debug.Assert(entry.CreatedByMaterializer, "entry.CreatedByMaterializer -- otherwise we shouldn't be calling this");

            if (ShouldTrackWithContext(entry))
            {
                this.identityStack.Add(entry.Identity, entry);
                if (this.mergeOption == MergeOption.AppendOnly)
                {
                    this.appendOnlyEntries.Add(entry.Identity, entry);
                }
            }
        }

        /// <summary>
        /// Invoke this method to notify the log that a link was removed 
        /// from a collection.
        /// </summary>
        /// <param name="source">
        /// Instance with the collection from which <paramref name="target"/> 
        /// was removed.
        /// </param>
        /// <param name="propertyName">Property name for collection.</param>
        /// <param name="target">Object which was removed.</param>
        internal void RemovedLink(AtomEntry source, string propertyName, object target)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(propertyName != null, "propertyName != null");

            if (ShouldTrackWithContext(source) && ShouldTrackWithContext(target))
            {
                Debug.Assert(this.Tracking, "this.Tracking -- otherwise there's an 'if' missing (it happens to be that the assert holds for all current callers");
                LinkDescriptor item = new LinkDescriptor(source.ResolvedObject, propertyName, target, EntityStates.Detached);
                this.links.Add(item);
            }
        }

        /// <summary>
        /// Invoke this method to notify the log that a link was set on
        /// a property.
        /// </summary>
        /// <param name="source">Entry for source object.</param>
        /// <param name="propertyName">Name of property set.</param>
        /// <param name="target">Target object.</param>
        internal void SetLink(AtomEntry source, string propertyName, object target)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(propertyName != null, "propertyName != null");

            if (!this.Tracking)
            {
                return;
            }

            if (ShouldTrackWithContext(source) && ShouldTrackWithContext(target))
            {
                Debug.Assert(this.Tracking, "this.Tracking -- otherwise there's an 'if' missing (it happens to be that the assert holds for all current callers");
                LinkDescriptor item = new LinkDescriptor(source.ResolvedObject, propertyName, target, EntityStates.Modified);
                this.links.Add(item);
            }
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Returns true if we should track this entry with context
        /// </summary>
        /// <param name="entry">The atom entry</param>
        /// <returns>true if entry should be tracked</returns>
        private static bool ShouldTrackWithContext(AtomEntry entry)
        {
            Debug.Assert(entry.ActualType != null, "Entry with no type added to log");
            return entry.ActualType.IsEntityType;
        }

        /// <summary>
        /// Returns true if we should track this entity with context
        /// </summary>
        /// <param name="entity">The resolved instance</param>
        /// <returns>true if entry should be tracked</returns>
        private static bool ShouldTrackWithContext(object entity)
        {
            if (entity == null)
            {
                // you can set link to null, we need to track these values
                return true;
            }

            ClientType type = ClientType.Create(entity.GetType());
            return type.IsEntityType;
        }

        /// <summary>
        /// Applies the media entry information (if any) to the specified 
        /// <paramref name="descriptor"/>.
        /// </summary>
        /// <param name="entry">Entry with (potential) media entry information to apply.</param>
        /// <param name="descriptor">Descriptor to update.</param>
        private void ApplyMediaEntryInformation(AtomEntry entry, EntityDescriptor descriptor)
        {
            Debug.Assert(entry != null, "entry != null");
            Debug.Assert(descriptor != null, "descriptor != null");

            if (entry.MediaEditUri != null || entry.MediaContentUri != null)
            {
                // 

                if (entry.MediaEditUri != null)
                {
                    descriptor.EditStreamUri = new Uri(this.context.BaseUriWithSlash, entry.MediaEditUri);
                }

                if (entry.MediaContentUri != null)
                {
                    descriptor.ReadStreamUri = new Uri(this.context.BaseUriWithSlash, entry.MediaContentUri);
                }

                descriptor.StreamETag = entry.StreamETagText;
            }
        }

        #endregion Private methods.
    }
}
