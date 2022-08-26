//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: ListItemParagraph.cs
//
// Description: ListItemParagraph represents a single list item.
//
// History:  
//  06/01/2004 : Microsoft - moving from Avalon branch.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Documents;
using MS.Internal.Text;

namespace MS.Internal.PtsHost
{

    /// <summary>
    /// ListItemParagraph represents a single list item.
    /// </summary>
    internal sealed class ListItemParagraph : ContainerParagraph
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="element">
        /// Element associated with paragraph.
        /// </param>
        /// <param name="structuralCache">
        /// Content's structural cache
        /// </param>
        internal ListItemParagraph(DependencyObject element, StructuralCache structuralCache)
            : base(element, structuralCache)
        {
        }
    }
}
