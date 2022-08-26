//---------------------------------------------------------------------------
//
// <copyright file=RightsManagementPermission.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Rights Managment Permission. 
//                  It is a class for permission that will be asserted/demanded internally. 
//                  Only DocumentApplication (or Mongoose) code will assert these permissions.
//
//              Using it allows the following: 
//                  We can have very specific targeted asserts for enabling Rights Management.
//                  This is to provide a granular permissio for Rights Management to be used
//                  by DocumentApplication to enable Encrypted Documents scenarios in Partial Trust
//                  rather than asserting broader permission such as Unmanaged Code
//
// !!!! Warning !!!!: No code other than DocumentApplication (or Mongoose) should assert this
//                      permission without agreement from this code owners.
// 
// History:  
//  09/12/05 : Microsoft - Created
//---------------------------------------------------------------------------

using System;
using System.Text;
using System.Security;
using System.Security.Permissions;
using System.Windows; 
using MS.Internal.WindowsBase;

namespace MS.Internal.Permissions
{
    // !!!! Warning !!!!: No code other than DocumentApplication (or Mongoose) should assert this
    //  permission without agreement from this code owners.
    [Serializable]
    [FriendAccessAllowed]
    internal class RightsManagementPermission : InternalParameterlessPermissionBase
    {
        public RightsManagementPermission() : this(PermissionState.Unrestricted)
        {
        }

        public RightsManagementPermission(PermissionState state): base(state)
        {
        }

        public override IPermission Copy()
        {
            // There is no state: just return a new instance of RightsManagementPermission
            return new RightsManagementPermission(); 
        }        
    }
        
}

