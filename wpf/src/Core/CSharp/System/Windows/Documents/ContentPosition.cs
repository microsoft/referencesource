//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: ContentPosition.cs
//
// Description: Represents a certain content's position. This position is 
//              content specific. 
//
// History:  
//  05/05/2003 : Microsoft - created.
//  09/28/2004 : Microsoft - updated pagination model.
//  08/29/2005 : Microsoft - updated pagination model.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents 
{
    /// <summary>
    /// Represents a certain content's position. This position is content 
    /// specific.
    /// </summary>
    public abstract class ContentPosition
    {
        /// <summary>
        /// Static representation of a non-existent ContentPosition.
        /// </summary>
        public static readonly ContentPosition Missing = new MissingContentPosition();

        #region Missing

        /// <summary>
        /// Representation of a non-existent ContentPosition.
        /// </summary>
        private class MissingContentPosition : ContentPosition {}

        #endregion Missing
    }
}
