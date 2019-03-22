//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace System.Activities.Presentation.View
{
    using System;
    using System.Diagnostics.CodeAnalysis;

    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = false)]
    [SuppressMessage("Microsoft.Design", "CA1019:DefineAccessorsForAttributeArguments")]
    public sealed class ViewVisibleAttribute : Attribute
    {
        public ViewVisibleAttribute()
        {
        }

        /// <summary>
        /// Constractor for ViewVisibleAttribute with promotedProperty parameter
        /// </summary>
        /// <param name="promotedProperty">
        /// This specify the name of the ModelItem/s to be shown in place of the
        /// current ModelItem
        /// </param>
        public ViewVisibleAttribute(string promotedProperty)
        {
            this.PromotedProperty = promotedProperty;
        }

        /// <summary>
        /// This specify the name of the ModelItem/s to be shown in place of the current
        /// ModelItem
        /// </summary>    
        public string PromotedProperty { get; set; }
    }
}
