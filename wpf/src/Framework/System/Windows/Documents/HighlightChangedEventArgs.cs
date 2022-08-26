//---------------------------------------------------------------------------
//
// <copyright file=”HighlightChangedEventArgs.cs” company=”Microsoft”>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: HighlightLayer.Changed event argument.
//
// History:  
//  07/01/2004 : benwest - Created
//
//---------------------------------------------------------------------------

using System.Collections;

namespace System.Windows.Documents
{
    /// <summary>
    /// HighlightLayer.Changed event argument.
    /// </summary>
    internal abstract class HighlightChangedEventArgs
    {
        /// <summary>
        /// Sorted, non-overlapping, readonly collection of TextSegments
        /// affected by a highlight change.
        /// </summary>
        internal abstract IList Ranges { get; }

        /// <summary>
        /// Type identifying the owner of the changed layer.
        /// </summary>
        internal abstract Type OwnerType { get; }
    }
}
