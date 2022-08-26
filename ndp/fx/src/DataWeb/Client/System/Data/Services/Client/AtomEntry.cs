//---------------------------------------------------------------------
// <copyright file="AtomEntry.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Provides a class to represent an ATOM entry as parsed and as it
// goes through the materialization pipeline.
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
    /// Use this class to represent an entry in an ATOM payload as it
    /// goes through the WCF Data Services materialization pipeline.
    /// </summary>
    /// <remarks>
    /// Different properties are set on instances of this type at different
    /// points during its lifetime.
    /// </remarks>
    [DebuggerDisplay("AtomEntry {ResolvedObject} @ {Identity}")]
    internal class AtomEntry
    {
        #region Private fields.

        /// <summary>Entry flags.</summary>
        private EntryFlags flags;

        /// <summary>
        /// Masks used get/set the status of the entry
        /// </summary>
        [Flags]
        private enum EntryFlags
        {
            /// <summary>Bitmask for ShouldUpdateFromPayload flag.</summary>
            ShouldUpdateFromPayload = 0x01,
            
            /// <summary>Bitmask for CreatedByMaterializer flag.</summary>
            CreatedByMaterializer = 0x02,

            /// <summary>Bitmask for EntityHasBeenResolved flag.</summary>
            EntityHasBeenResolved = 0x04,

            /// <summary>Bitmask for MediaLinkEntry flag (value).</summary>
            MediaLinkEntryValue = 0x08,

            /// <summary>Bitmask for MediaLinkEntry flag (assigned/non-null state).</summary>
            MediaLinkEntryAssigned = 0x10,

            /// <summary>Bitmask for EntityPropertyMappingsApplied flag.</summary>
            EntityPropertyMappingsApplied = 0x20,

            /// <summary>Bitmask for IsNull flag.</summary>
            IsNull = 0x40
        }

        #endregion Private fields.

        #region Public properties.

        /// <summary>MediaLinkEntry. Valid after data values applies.</summary>
        public bool? MediaLinkEntry
        {
            get 
            {
                return this.GetFlagValue(EntryFlags.MediaLinkEntryAssigned) ? (bool?)this.GetFlagValue(EntryFlags.MediaLinkEntryValue) : null; 
            }

            set 
            {
                Debug.Assert(value.HasValue, "value.HasValue -- callers shouldn't set the value to unknown");
                this.SetFlagValue(EntryFlags.MediaLinkEntryAssigned, true);
                this.SetFlagValue(EntryFlags.MediaLinkEntryValue, value.Value);
            }
        }

        /// <summary>URI for media content. null if MediaLinkEntry is false.</summary>
        public Uri MediaContentUri 
        { 
            get; 
            set; 
        }

        /// <summary>URI for editing media. null if MediaLinkEntry is false.</summary>
        public Uri MediaEditUri 
        { 
            get; 
            set; 
        }

        /// <summary>Type name, as present in server payload.</summary>
        public string TypeName 
        { 
            get; 
            set; 
        }

        /// <summary>Actual type of the ResolvedObject.</summary>
        public ClientType ActualType 
        { 
            get; 
            set; 
        }

        /// <summary>Edit link for the ATOM entry - basically link used to update the entity.</summary>
        public Uri EditLink 
        { 
            get; 
            set; 
        }

        /// <summary>Self link for the ATOM entry - basically link used to query the entity.</summary>
        public Uri QueryLink
        {
            get;
            set;
        }

        /// <summary>
        /// Identity link for the ATOM entry. 
        /// This is set by the parser (guaranteed, fails to parser if not available on entry).
        /// </summary>
        public string Identity 
        { 
            get; 
            set; 
        }

        /// <summary>
        /// Whether the entry is a null.
        /// This is set by the parser only on inlined entries, as it's invalid ATOM in
        /// top-level cases.
        /// </summary>
        public bool IsNull
        {
            get { return this.GetFlagValue(EntryFlags.IsNull); }
            set { this.SetFlagValue(EntryFlags.IsNull, value); }
        }

        /// <summary>Data names and values.</summary>
        public List<AtomContentProperty> DataValues 
        { 
            get; 
            set; 
        }

        /// <summary>Resolved object.</summary>
        public object ResolvedObject 
        { 
            get; 
            set; 
        }

        /// <summary>Tag for the entry, set by the parser when an entry callback is invoked.</summary>
        public object Tag 
        { 
            get; 
            set; 
        }

        /// <summary>Text for Etag, possibly null.</summary>
        public string ETagText 
        { 
            get; 
            set; 
        }

        /// <summary>Text for ETag corresponding to media resource for this entry.</summary>
        public string StreamETagText
        {
            get;
            set;
        }

        /// <summary>Whether values should be updated from payload.</summary>
        public bool ShouldUpdateFromPayload
        {
            get { return this.GetFlagValue(EntryFlags.ShouldUpdateFromPayload); }
            set { this.SetFlagValue(EntryFlags.ShouldUpdateFromPayload, value); }
        }

        /// <summary>Whether the materializer has created the ResolvedObject instance.</summary>
        public bool CreatedByMaterializer
        {
            get { return this.GetFlagValue(EntryFlags.CreatedByMaterializer); }
            set { this.SetFlagValue(EntryFlags.CreatedByMaterializer, value); }
        }

        /// <summary>Whether the entity has been resolved / created.</summary>
        public bool EntityHasBeenResolved
        {
            get { return this.GetFlagValue(EntryFlags.EntityHasBeenResolved); }
            set { this.SetFlagValue(EntryFlags.EntityHasBeenResolved, value); }
        }

        /// <summary>Whether entity Property Mappings (a.k.a. friendly feeds) have been applied to this entry if applicable.</summary>
        public bool EntityPropertyMappingsApplied
        {
            get { return this.GetFlagValue(EntryFlags.EntityPropertyMappingsApplied); }
            set { this.SetFlagValue(EntryFlags.EntityPropertyMappingsApplied, value); }
        }

        #endregion Public properties.

        #region Private methods.

        /// <summary>Gets the value for a masked item.</summary>
        /// <param name="mask">Mask value.</param>
        /// <returns>true if the flag is set; false otherwise.</returns>
        private bool GetFlagValue(EntryFlags mask)
        {
            return (this.flags & mask) != 0;
        }

        /// <summary>Sets the value for a masked item.</summary>
        /// <param name="mask">Mask value.</param>
        /// <param name="value">Value to set</param>
        private void SetFlagValue(EntryFlags mask, bool value)
        {
            if (value)
            {
                this.flags |= mask;
            }
            else
            {
                this.flags &= (~mask);
            }
        }

        #endregion Private methods.
    }
}
