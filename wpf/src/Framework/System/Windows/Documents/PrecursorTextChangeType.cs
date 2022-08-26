//---------------------------------------------------------------------------
//
// <copyright file=PrecursorTextChangeType.cs company=Microsoft>
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
    // These are possible changes added to a change list.
    // ElementAdded/Extracted don't make sense after multiple
    // changes are combined.
    internal enum PrecursorTextChangeType
    { 
        ContentAdded = TextChangeType.ContentAdded,
        ContentRemoved = TextChangeType.ContentRemoved,
        PropertyModified = TextChangeType.PropertyModified,
        ElementAdded,
        ElementExtracted
    }
}
