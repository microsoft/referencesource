//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace System.Activities.Presentation.View
{
    using System;
    using System.Diagnostics.CodeAnalysis;

    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field, Inherited = true, AllowMultiple = false)]
    [SuppressMessage("Microsoft.Design", "CA1019:DefineAccessorsForAttributeArguments")]
    public sealed class ViewChildAttribute : Attribute
    {
        public ViewChildAttribute()
            : this(true)
        {
        }

        /// <summary>
        /// Constructor for ViewChildAttribute with propertyNodeVisible property
        /// </summary>
        /// <param name="propertyNodeVisible">
        /// Include the property node for this property
        /// </param>
        public ViewChildAttribute(bool propertyNodeVisible)
        {
            this.PropertyNodeVisible = propertyNodeVisible;
            this.DuplicatedNodesVisible = true;
        }

        /// <summary>
        /// Visibility of the property node
        /// </summary>        
        public bool PropertyNodeVisible { get; private set; }
        
        /// <summary>
        /// If false, we'll skip children that are visible elsewhere in the tree view
        /// </summary>
        public bool DuplicatedNodesVisible { get; set; }

        /// <summary>
        /// Prefix text into its child nodes.
        /// </summary>
        public string ChildNodePrefix { get; set; }
    }
}
