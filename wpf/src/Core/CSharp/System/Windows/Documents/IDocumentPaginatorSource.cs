//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: IDocumentPaginatorSource.cs
//
// Description: This interface is implemented by elements that support 
//              paginated layout of their content. IDocumentPaginatorSource 
//              contains only one member, DocumentPaginator, the object which 
//              performs the actual pagination of content.
//
// History:  
//  08/29/2005 : grzegorz - created.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents 
{
    /// <summary>
    /// This interface is implemented by elements that support paginated 
    /// layout of their content. IDocumentPaginatorSource contains only 
    /// one member, DocumentPaginator, the object which performs the actual 
    /// pagination of content.
    /// </summary>
    public interface IDocumentPaginatorSource
    {
        /// <summary>
        /// An object which paginates content.
        /// </summary>
        DocumentPaginator DocumentPaginator { get; }
    }
}
