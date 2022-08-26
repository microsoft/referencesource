//---------------------------------------------------------------------------
//
// <copyright file="ICollectionViewFactory.cs" company="Microsoft">
//    Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Factory interface to create views on collections.
//
// See spec at http://avalon/connecteddata/Specs/CollectionView.mht
// 
// History:  
//  05/29/2003 : davidjen - Created
//
//---------------------------------------------------------------------------

using System;

namespace System.ComponentModel
{
    /// <summary>
    /// Allows an implementing collection to create a view to its data.
    /// Normally, user code does not call methods on this interface.
    /// </summary>
    public interface ICollectionViewFactory
    {
        /// <summary>
        /// Create a new view on this collection [Do not call directly].
        /// </summary>
        /// <remarks>
        /// Normally this method is only called by the platform's view manager,
        /// not by user code.
        /// </remarks>
        ICollectionView CreateView();
    }
}

