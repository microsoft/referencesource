//------------------------------------------------------------------------------
// <copyright file="IMessageFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Remoting;

    using System.Diagnostics;
    
    using System;
	using System.Security;
	using System.Security.Permissions;

    /// <include file='doc\IMessageFilter.uex' path='docs/doc[@for="IMessageFilter"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Defines a message filter interface.</para>
    /// </devdoc>
    public interface IMessageFilter {
    
        /// <include file='doc\IMessageFilter.uex' path='docs/doc[@for="IMessageFilter.PreFilterMessage"]/*' />
        /// <devdoc>
        ///    <para>Filters out a message before it is dispatched. </para>
        /// </devdoc>
        /// SECREVIEW:
        ///         
        /// The link demand on the interface method isn't sufficient to prevent a partial trust caller from re-implementing this interface on subclasses 
        /// of Framework object that already implement this interface and utilize AddMessageFilter. 
        /// As a result, the subclass's implementation will get called by the message pump instead of the base class's implementation. 
        /// This would allow untrusted code to start examining message traffic or filter out any messages at whim.So please be careful while implementing this Interface.
        /// Please refer to VsWhidbey : 423553 for more details.
        ///
		[
		System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
		]
		bool PreFilterMessage(ref Message m);
    }
}
