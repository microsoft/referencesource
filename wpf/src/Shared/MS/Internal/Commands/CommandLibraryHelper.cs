//---------------------------------------------------------------------------
//
// <copyright file=CommandLibraryHelper.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Set of helpers used by all Commands. 
//
// History:  
//  02/28/2004 : marka - Created
//
//---------------------------------------------------------------------------

using System; 
using System.Security; 
using System.Security.Permissions; 
using System.Windows.Input; 


using MS.Internal.PresentationCore; // for FriendAccessAllowed

namespace MS.Internal
{
    [FriendAccessAllowed]
    internal static class CommandLibraryHelper
    {
        internal static RoutedUICommand CreateUICommand(string name, Type ownerType, byte commandId, PermissionSet ps)
        {
            RoutedUICommand routedUICommand;
            
            if (ps != null)
            {
                routedUICommand = new SecureUICommand(ps, name, ownerType, commandId);
            }
            else
            {
                routedUICommand = new RoutedUICommand(name, ownerType, commandId);
            }

            routedUICommand.AreInputGesturesDelayLoaded = true;
            return routedUICommand;
        }                        
     }
}     
