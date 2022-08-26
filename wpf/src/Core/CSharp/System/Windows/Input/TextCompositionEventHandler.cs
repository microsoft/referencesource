//---------------------------------------------------------------------------
//
// <copyright file=TextCompositionEventHandler.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description: TextCompositionEventHandler delegate
//
// History:  
//  11/18/2003 : yutakas created
//
//---------------------------------------------------------------------------

using System;
namespace System.Windows.Input 
{
    /// <summary>
    ///     The delegate to use for handlers that receive TextCompositionEventArgs.
    /// </summary>
    /// <ExternalAPI Inherit="true"/>
    public delegate void TextCompositionEventHandler(object sender, TextCompositionEventArgs e);
}

