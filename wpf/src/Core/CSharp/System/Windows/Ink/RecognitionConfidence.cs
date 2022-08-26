//---------------------------------------------------------------------------
//
// File: ApplicationGesture.cs
//
// Description:
//      The definition of RecognitionConfidence enum type
//
// Features:
//
// History:
//  01/14/2005 waynezen:       Created
//
// Copyright (C) 2001 by Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Ink
{
    /// <summary>
    /// RecognitionConfidence
    /// </summary>
    public enum RecognitionConfidence
    {
        /// <summary>
        /// Strong
        /// </summary>
        Strong = 0,
        /// <summary>
        /// Intermediate
        /// </summary>
        Intermediate = 1,
        /// <summary>
        /// Poor
        /// </summary>
        Poor = 2,
    }
}