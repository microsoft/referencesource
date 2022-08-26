//---------------------------------------------------------------------------
//
// File: TextContainerChangeEventHandler.cs
//
// Description: Delegate for a Change event fired on a TextContainer.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace System.Windows.Documents
{
    // Delegate for a ChangeAdded event fired on a TextContainer.
    internal delegate void TextContainerChangeEventHandler(object sender, TextContainerChangeEventArgs e);
}
