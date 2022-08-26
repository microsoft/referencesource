//---------------------------------------------------------------------------
//
// File: TextContainerChangedEventHandler.cs
//
// Description: Delegate for a TextChangedEvent fired on a TextContainer.
//
// History:  
//  8/3/2004 : benwest - Created - preparing for TextContainer eventing
//             breaking change.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace System.Windows.Documents
{
    /// <summary>
    ///  The TextChangedEventHandler delegate is called with TextContainerChangedEventArgs every time
    ///  content is added to or removed from the TextContainer
    /// </summary>
    internal delegate void TextContainerChangedEventHandler(object sender, TextContainerChangedEventArgs e);
}
