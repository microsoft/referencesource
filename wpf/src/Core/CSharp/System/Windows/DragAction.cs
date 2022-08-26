//---------------------------------------------------------------------------
//
// File: DragAction.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// Description: Specifies how and if a drag-and-drop operation should continue.
//
// 
// History:  
//  08/19/2004 : sangilj    Created
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows
{
    /// <summary>
    /// An enumeration of the DragDropResult that the DragSource will return from 
    /// QueryContinueDrag event handler or GiveFeedback handler.
    /// </summary>
    public enum DragAction
    {
        /// <summary>
        /// The DragDrop can continue.  
        /// Return by QueryContinueDrag    
        /// </summary>
        Continue = 0,
        /// <summary>
        /// Drop operation should occur, 
        /// Return by QueryContinueDrag    
        /// </summary>
        Drop = 1,
        /// <summary>
        /// Drop operation is canceled  
        /// Return by QueryContinueDrag    
        /// </summary>
        Cancel = 2,
    }
}

