//---------------------------------------------------------------------------
//
// <copyright file="StyleSelector.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: StyleSelector allows the app writer to provide custom style selection logic.
//
// Specs:       http://avalon/connecteddata/M5%20General%20Docs/Data%20Styling.mht
//
//---------------------------------------------------------------------------

using System.Windows.Shapes;
using System.Windows.Media;
using System.Windows.Data;
using System.ComponentModel;


using System;

namespace System.Windows.Controls
{
    /// <summary>
    /// <p>
    /// StyleSelector allows the app writer to provide custom style selection logic.
    /// For example, with a class Bug as the Content,
    /// use a particular style for Pri1 bugs and a different style for Pri2 bugs.
    /// </p>
    /// <p>
    /// An application writer can override the SelectStyle method in a derived
    /// selector class and assign an instance of this class to the StyleSelector property on
    /// <seealso cref="ContentPresenter"/> class.
    /// </p>
    /// </summary>
    public class StyleSelector
    {
        /// <summary>
        /// Override this method to return an app specific <seealso cref="Style"/>.
        /// </summary>
        /// <param name="item">The data content</param>
        /// <param name="container">The element to which the style will be applied</param>
        /// <returns>an app-specific style to apply, or null.</returns>
        public virtual Style SelectStyle(object item, DependencyObject container)
        {
            return null;
        }
    }
}
