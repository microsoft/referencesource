//---------------------------------------------------------------------------
//
// File: DragEventHandler.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// Description: DragEventHandler for DragEnter/DragOver/DragLeave/Drop events.
// 
// History:  
//  08/19/2004 : sangilj    Created
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows
{
    /// <summary>
    /// The delegate to use for handlers that receive DragEventArgs.
    /// </summary>
    public delegate void DragEventHandler(object sender, DragEventArgs e);
}

