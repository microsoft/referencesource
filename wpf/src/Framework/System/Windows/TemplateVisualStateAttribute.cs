// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------

using System;

namespace System.Windows
{
    /// <summary>
    ///     Define an expected VisualState in the contract between a Control and its
    ///     ControlTemplate for use with the VisualStateManager.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public sealed class TemplateVisualStateAttribute : Attribute
    {
        /// <summary>
        ///     Name of the VisualState.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        /// <summary>
        ///     Name of the VisualStateGroup containing this state.
        /// </summary>
        public string GroupName
        {
            get;
            set;
        }
    }
}
