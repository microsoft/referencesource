//---------------------------------------------------------------------------
//
// <copyright file="IAsyncDataDispatcher.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description: Interface for the asynchronous data dispatcher.
//
// Specs:       http://avalon/connecteddata/M5%20Specs/Asynchronous%20Data%20Model.mht
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Data;

namespace MS.Internal.Data
{
    /// <summary> Interface for the asynchronous data dispatcher. </summary>
    internal interface IAsyncDataDispatcher
    {
        /// <summary> Add a request to the dispatcher's queue </summary>
        void AddRequest(AsyncDataRequest request);      // thread-safe

        /// <summary> Cancel all requests in the dispatcher's queue </summary>
        void CancelAllRequests();                       // thread-safe
    }
}
