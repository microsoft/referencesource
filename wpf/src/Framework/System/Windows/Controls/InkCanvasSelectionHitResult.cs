//---------------------------------------------------------------------------
//
// File: InkCanvasSelectionHandle.cs
//
// Description:
//      Defines an enum type of the InkCanvas selection handle
//
// Features:
//
// History:
//  1/27/2005 waynezen:       Created 
//
// Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    /// InkCanvas Selection Hit Result
    /// </summary>
    public enum InkCanvasSelectionHitResult
    {
        /// <summary>
        /// None
        /// </summary>
        None =          0,
        /// <summary>
        /// TopLeft
        /// </summary>
        TopLeft =       1,
        /// <summary>
        /// Top
        /// </summary>
        Top =           2,
        /// <summary>
        /// TopRight
        /// </summary>
        TopRight =      3,
        /// <summary>
        /// Right
        /// </summary>
        Right =         4,
        /// <summary>
        /// BottomRight
        /// </summary>
        BottomRight =   5,
        /// <summary>
        /// Bottom
        /// </summary>
        Bottom =        6,
        /// <summary>
        /// BottomLeft
        /// </summary>
        BottomLeft =    7,
        /// <summary>
        /// Left
        /// </summary>
        Left =          8,
        /// <summary>
        /// Selection
        /// </summary>
        Selection =     9,
    }
}