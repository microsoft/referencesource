// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
//  DataProtectionPermission.cs
//

namespace System.Security.Permissions {
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()]
    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class DataProtectionPermissionAttribute : CodeAccessSecurityAttribute {
        private DataProtectionPermissionFlags m_flags = DataProtectionPermissionFlags.NoFlags;

        public DataProtectionPermissionAttribute (SecurityAction action) : base (action) {}

        public DataProtectionPermissionFlags Flags {
            get { return m_flags; }
            set { 
                DataProtectionPermission.VerifyFlags(value);
                m_flags = value; 
            }
        }

        public bool ProtectData {
            get { return (m_flags & DataProtectionPermissionFlags.ProtectData) != 0; }
            set { m_flags = value ? m_flags | DataProtectionPermissionFlags.ProtectData : m_flags & ~DataProtectionPermissionFlags.ProtectData; }
        }

        public bool UnprotectData {
            get { return (m_flags & DataProtectionPermissionFlags.UnprotectData) != 0; }
            set { m_flags = value ? m_flags | DataProtectionPermissionFlags.UnprotectData : m_flags & ~DataProtectionPermissionFlags.UnprotectData; }
        }

        public bool ProtectMemory {
            get { return (m_flags & DataProtectionPermissionFlags.ProtectMemory) != 0; }
            set { m_flags = value ? m_flags | DataProtectionPermissionFlags.ProtectMemory : m_flags & ~DataProtectionPermissionFlags.ProtectMemory; }
        }

        public bool UnprotectMemory {
            get { return (m_flags & DataProtectionPermissionFlags.UnprotectMemory) != 0; }
            set { m_flags = value ? m_flags | DataProtectionPermissionFlags.UnprotectMemory : m_flags & ~DataProtectionPermissionFlags.UnprotectMemory; }
        }

        public override IPermission CreatePermission () {
            if (Unrestricted)
                return new DataProtectionPermission(PermissionState.Unrestricted);
            else 
                return new DataProtectionPermission(m_flags);
        }
    }
}
