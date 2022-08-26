//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// Description: LineBreak element. 
//
// History:  
//  07/22/2002 : MikeOrr - Created.
//  06/26/2003 : ZhenbinX - Ported to /Rewrote for WCP tree
//  10/28/2004 : Microsoft - ContentElements refactoring.
//
//---------------------------------------------------------------------------

using System.Windows.Markup; // TrimSurrondingWhitespace

namespace System.Windows.Documents 
{
    /// <summary>
    /// LineBreak element that forces a line breaking. 
    /// </summary>
    [TrimSurroundingWhitespace]
    public class LineBreak : Inline
    {
        /// <summary>
        /// Creates a new LineBreak instance.
        /// </summary>
        public LineBreak()
        {
        }

        /// <summary>
        /// Creates a new LineBreak instance.
        /// </summary>
        /// <param name="insertionPosition">
        /// Optional position at which to insert the new LineBreak. May
        /// be null.
        /// </param>
        public LineBreak(TextPointer insertionPosition)
        {
            if (insertionPosition != null)
            {
                insertionPosition.TextContainer.BeginChange();
            }
            try
            {
                if (insertionPosition != null)
                {
                    // This will throw InvalidOperationException if schema validity is violated.
                    insertionPosition.InsertInline(this);
                }
            }
            finally
            {
                if (insertionPosition != null)
                {
                    insertionPosition.TextContainer.EndChange();
                }
            }
        }
    }
}

