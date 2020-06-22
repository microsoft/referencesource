//------------------------------------------------------------------------------
// <copyright file="PeekCompletedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;

    // <summary>
    //    Represents the signature of the callback that will
    //    be executed when an asynchronous message queue
    //    peek operation is completed.
    // </summary>
    // <param name='sender'>
    //    Contains the MessageQueue object that calls the method.
    // </param>
    // <param name='args'>
    //    The event information associated with the call.
    // </param>
    // </doc>
    //    
    /// <include file='doc\PeekCompletedEventHandler.uex' path='docs/doc[@for="PeekCompletedEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.Messaging.MessageQueue.PeekCompleted'/> event of a <see cref='System.Messaging.MessageQueue'/>.</para>
    /// </devdoc>
    public delegate void PeekCompletedEventHandler(object sender, PeekCompletedEventArgs e);

}
