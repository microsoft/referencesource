//---------------------------------------------------------------------------
//
// <copyright file="InternalCollectionDelegates.cs" company="Microsoft">
//    Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Event handlers used internally to track changes to our
//              generated collection classes.
//              
// History:  
//  5/20/2004 : danlehen - Created
//
//---------------------------------------------------------------------------

namespace MS.Internal.Collections
{
    internal delegate void ItemInsertedHandler(object sender, object item);
    internal delegate void ItemRemovedHandler(object sender, object item);
}

