//---------------------------------------------------------------------------
//
// <copyright file=TextChange.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: 
//
// History:  
//  8/3/2004 : benwest - Created - preparing for TextContainer eventing
//             breaking change.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Collections;

namespace System.Windows.Documents
{
    /// <summary>
    /// Specifies the type of change applied to TextContainer content.
    /// </summary>
    internal enum TextChangeType
    { 
        /// <summary>
        /// New content was inserted.
        /// </summary>
        ContentAdded,

        /// <summary>
        /// Content was deleted.
        /// </summary>
        ContentRemoved, 

        /// <summary>
        /// A local DependencyProperty value changed.
        /// </summary>
        PropertyModified,
    }
}
