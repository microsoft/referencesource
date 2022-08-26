/////////////////////////////////////////////////////////////////////////////
//
// File: LogicalDirection.cs
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
//
// Description: LogicalDirection defines a logical direction for movement in text
//
/////////////////////////////////////////////////////////////////////////////

namespace System.Windows.Documents
{
    /// <summary>
    ///  LogicalDirection defines a logical direction for movement in text.  It 
    ///  is also used to determine where a TextPointer will move when content
    ///  is inserted at the TextPointer.  
    /// </summary>
    public enum LogicalDirection
    {
        /// <summary>
        ///  Backward - Causes the TextPointer to be positioned 
        ///  before the newly inserted content
        /// </summary>
        Backward            = 0,
        /// <summary>
        ///  Forward - Causes the TextPointer 
        ///  to be positioned after the newly inserted content.
        /// </summary>
        Forward             = 1
    }
}
