//---------------------------------------------------------------------
// <copyright file="ResourceContainer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Contains information about a resource set.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Reflection;

    /// <summary>
    /// Structure to keep information about a resource set
    /// </summary>
    /// <remarks>
    /// Custom providers can choose to use it as is or derive from it
    /// in order to flow provider-specific data.
    /// </remarks>
    [DebuggerDisplay("{Name}: {ResourceType}")]
    public class ResourceSet
    {
        #region Fields
        /// <summary> Reference to resource type that this resource set is a collection of</summary>
        private readonly ResourceType elementType;

        /// <summary>Name of the resource set.</summary>
        private readonly string name;

        /// <summary>Cached delegate to read an IQueryable from the context.</summary>
        private Func<object, System.Linq.IQueryable> readFromContextDelegate;

        /// <summary>Is true, if the resource set is fully initialized and validated. No more changes can be made once its set to readonly.</summary>
        private bool isReadOnly;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructs a new ResourceSet instance using the specified name and ResourceType instance
        /// </summary>
        /// <param name="name">name of the resource set</param>
        /// <param name="elementType">Reference to clr type that this resource set is a collection of</param>
        public ResourceSet(string name, ResourceType elementType)
        {
            WebUtil.CheckStringArgumentNull(name, "name");
            WebUtil.CheckArgumentNull(elementType, "elementType");

            if (elementType.ResourceTypeKind != ResourceTypeKind.EntityType)
            {
                throw new ArgumentException(Strings.ResourceContainer_ContainerMustBeAssociatedWithEntityType);
            }

            this.name = name;
            this.elementType = elementType;
        }

        #endregion Constructors

        #region Properties

        /// <summary>Name of the resource set.</summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary> Reference to resource type that this resource set is a collection of </summary>
        public ResourceType ResourceType
        {
            get { return this.elementType; }
        }

        /// <summary>
        /// PlaceHolder to hold custom state information about resource set.
        /// </summary>
        public object CustomState
        {
            get;
            set;
        }

        /// <summary>
        /// Returns true, if this container has been set to read only. Otherwise returns false.
        /// </summary>
        public bool IsReadOnly
        {
            get { return this.isReadOnly; }
        }

        /// <summary>Cached delegate to read an IQueryable from the context.</summary>
        internal Func<object, System.Linq.IQueryable> ReadFromContextDelegate
        {
            get { return this.readFromContextDelegate; }
            set { this.readFromContextDelegate = value; }
        }

        #endregion Properties

        #region Methods
        /// <summary>
        /// Sets the resource set to readonly mode. resource sets cannot be updated once this property is set.
        /// </summary>
        public void SetReadOnly()
        {
            // If its already set to readonly, then its a no-op
            if (this.isReadOnly)
            {
                return;
            }

            this.elementType.SetReadOnly();
            this.isReadOnly = true;
        }

        #endregion Methods
    }
}
