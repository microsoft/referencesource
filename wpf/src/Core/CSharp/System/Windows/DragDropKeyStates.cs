//---------------------------------------------------------------------------
//
// File: DragDropKeyStates.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// Description: Specifies the key and mouse states for drag-and-drop operation.
// 
// History:  
//  08/19/2004 : sangilj    Created
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows
{
    /// <summary>
    /// An enumeration of states of keyboard and mouse.
    /// </summary>
    [Flags]
    public enum DragDropKeyStates
    {
        /// <summary>
        /// No state set.
        /// </summary>
        None = 0,
        /// <summary>
        /// The left mouse button.  
        /// </summary>
        LeftMouseButton = 1,
        /// <summary>
        /// The right mouse button.   
        /// </summary>
        RightMouseButton = 2,
        /// <summary>
        /// The SHIFT key.   
        /// </summary>
        ShiftKey = 4,
        /// <summary>
        /// The CTRL key.
        /// </summary>
        ControlKey = 8,
        /// <summary>
        /// The middle mouse button.
        /// </summary>
        MiddleMouseButton = 16,
        /// <summary>
        /// The ALT key.   
        /// </summary>
        AltKey = 32,
    } 
}

