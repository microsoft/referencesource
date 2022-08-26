//---------------------------------------------------------------------------
// <copyright file="ItemContainerTemplateSelector.cs" company="Microsoft Corporation">
//     Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.Diagnostics;
using System.Windows.Media;

namespace System.Windows.Controls
{
    /// <summary>
    ///   A class used to select an ItemContainerTemplate for each item within an ItemsControl
    /// </summary>
    public abstract class ItemContainerTemplateSelector
    {
        /// <summary>
        /// Override this method to return an app specific ItemContainerTemplate
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        public virtual DataTemplate SelectTemplate(object item, ItemsControl parentItemsControl)
        {
            return null;
        }
    }

    internal class DefaultItemContainerTemplateSelector : ItemContainerTemplateSelector
    {
        public override DataTemplate SelectTemplate(object item, ItemsControl parentItemsControl)
        {
            // Do an implicit type lookup for an ItemContainerTemplate
            return FrameworkElement.FindTemplateResourceInternal(parentItemsControl, item, typeof(ItemContainerTemplate)) as DataTemplate;
        }
    }
}
