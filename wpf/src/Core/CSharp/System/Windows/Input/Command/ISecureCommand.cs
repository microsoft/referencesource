//---------------------------------------------------------------------------
//
// <copyright file=ISecureCommand.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description: ISecureCommand enables a command to specify that calls
//              must have a specific permission to modify the bindings
//              associated with that command. That permission will
//              then be asserted when the command is invoked in a user
//              interactive (trusted) way.
//  
//              See spec at : (need link)
// 
//
// History:  
//      01/26/2005 : chrisan - Created
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Security;
using System.Security.Permissions;
using MS.Internal.PresentationCore;

namespace System.Windows.Input
{
    ///<summary>
    /// ISecureCommand enables a command to specify that calls
    /// must have a specific permission to modify the bindings
    /// associated with that command. That permission will
    /// then be asserted when the command is invoked in a user
    /// interactive (trusted) way.
    ///</summary>
    [FriendAccessAllowed]
    [TypeConverter("System.Windows.Input.CommandConverter, PresentationFramework, Version=" + BuildInfo.WCP_VERSION + ", Culture=neutral, PublicKeyToken=" + BuildInfo.WCP_PUBLIC_KEY_TOKEN + ", Custom=null")]
    internal interface ISecureCommand : ICommand
    {
        /// <summary>
        /// Permission required to modify bindings for this
        /// command, and the permission to assert when
        /// the command is invoked in a user interactive
        /// (trusted) fashion.
        /// </summary>
        PermissionSet UserInitiatedPermission { get; }
    }

}
