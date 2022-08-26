// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>ShawnFa</OWNER>
// 

//
//  DataProtectionPermissionFlags.cs
//

namespace System.Security.Permissions {
    [Flags, Serializable]
    public enum DataProtectionPermissionFlags {
        NoFlags         = 0x00,

        ProtectData     = 0x01,
        UnprotectData   = 0x02,

        ProtectMemory   = 0x04,
        UnprotectMemory = 0x08,

        AllFlags        = 0x0F
    }
}
