//---------------------------------------------------------------------
// <copyright file="Descriptor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// represents the response object - either entity or link
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    /// <summary>
    /// represents the response object - either entity or link
    /// </summary>
    public abstract class Descriptor
    {
        #region Fields

        /// <summary>change order</summary>
        private uint changeOrder = UInt32.MaxValue;

        /// <summary>was content generated for the entity</summary>
        private bool saveContentGenerated;

        /// <summary>was this entity save result processed</summary>
        /// <remarks>0 - no processed, otherwise reflects the previous state</remarks>
        private EntityStates saveResultProcessed;

        /// <summary>last save exception per entry</summary>
        private Exception saveError;

        /// <summary>State of the modified entity or link.</summary>
        private EntityStates state;

        #endregion

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="state">entity state</param>
        internal Descriptor(EntityStates state)
        {
            this.state = state;
        }

        #region Public Properties

        /// <summary>returns the state of the entity or link object in response.</summary>
        public EntityStates State
        {
            get { return this.state; }
            internal set { this.state = value; }
        }

        #endregion

        #region Internal Properties
        
        /// <summary>true if resource, false if link</summary>
        internal abstract bool IsResource
        {
            get;
        }

        /// <summary>changeOrder</summary>
        internal uint ChangeOrder
        {
            get { return this.changeOrder; }
            set { this.changeOrder = value; }
        }

        /// <summary>was content generated for the entity</summary>
        internal bool ContentGeneratedForSave
        {
            get { return this.saveContentGenerated; }
            set { this.saveContentGenerated = value; }
        }

        /// <summary>was this entity save result processed</summary>
        internal EntityStates SaveResultWasProcessed
        {
            get { return this.saveResultProcessed; }
            set { this.saveResultProcessed = value; }
        }

        /// <summary>last save exception per entry</summary>
        internal Exception SaveError
        {
            get { return this.saveError; }
            set { this.saveError = value; }
        }
        
        /// <summary>
        /// Returns true if the entry has been modified (and thus should participate in SaveChanges).
        /// </summary>
        internal virtual bool IsModified
        {
            get
            {
                System.Diagnostics.Debug.Assert(
                    (EntityStates.Added == this.state) ||
                    (EntityStates.Modified == this.state) ||
                    (EntityStates.Unchanged == this.state) ||
                    (EntityStates.Deleted == this.state),
                    "entity state is not valid");

                return (EntityStates.Unchanged != this.state);
            }
        }

        #endregion
    }
}
