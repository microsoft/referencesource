//---------------------------------------------------------------------------
//
// File: InkCanvasClipboardFormat.cs
//
// Description:
//      Defines an Enum type which is used by InkCanvas' Clipboard supports.
//
// Features:
//
// History:
//  08/19/2005 waynezen:    Created
//
// Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

namespace System.Windows.Controls
{
    /// <summary>
    /// InkCanvasClipboardFormat
    /// </summary>
    public enum InkCanvasClipboardFormat
    {
        /// <summary>
        /// Ink Serialized Format
        /// </summary>
        InkSerializedFormat = 0,
        /// <summary>
        /// Text Format
        /// </summary>
        Text,
        /// <summary>
        /// Xaml Format
        /// </summary>
        Xaml,
    }
}
