//-----------------------------------------------------------------------------
//
// <copyright file="UserActivationMode.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:  This enumeration is used as a parameter in SecureEnvironment.Create to  
//   indicate whether permanent or temporary certificate request needs to be issued.
//
// History:
//  06/01/2005: IgorBel :   Initial Implementation 
//
//-----------------------------------------------------------------------------

using System;

namespace System.Security.RightsManagement 
{

    /// <summary>
    /// This enumeration is used to indicate whether we are going to request temporary or permanent User Certificate from RM server
    /// </summary>
    public enum UserActivationMode : int
    {
        /// <summary>
        /// Permanent User Certificate will be requested
        /// </summary>
        Permanent = 0,
        
        /// <summary>
        /// Temporary User Certificate will be requested
        /// </summary>
        Temporary = 1,
    }
}
 
