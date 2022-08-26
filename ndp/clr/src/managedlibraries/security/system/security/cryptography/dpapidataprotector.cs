// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// <OWNER>jeffcoop</OWNER>
// <OWNER>Microsoft</OWNER>
//

using System;
using System.Security.Permissions;

namespace System.Security.Cryptography
{
    public sealed class DpapiDataProtector : DataProtector
    {
        public DataProtectionScope Scope { get; set; }

        // We will use the base class's HashedPurpose and pass it as OptionalEntropy to DPAPI
        // The default for DataProtector is to prepend the hash to the plain text, but we
        // do not need that because of the OptionalEntropy guarantees
        protected override bool PrependHashedPurposeToPlaintext 
        {
            get
            {
                return false;
            }
        }

        // NOTE: We want applications like ASP.NET to be able to hand out instances of a DataProtector
        // To do this, we demand unrestricted DataProtectionPermission in the constructor, but Assert
        // the permission when the work is actually done.  This makes the class behave similar to FileStream
        // where access is checked at time of creation, not time of use.
        [SecuritySafeCritical]
        [DataProtectionPermission(SecurityAction.Assert, ProtectData = true)]
        protected override byte[] ProviderProtect(byte[] userData)
        {
            // Delegate to ProtectedData
            return ProtectedData.Protect(userData, GetHashedPurpose(), Scope);
        }

        // NOTE: We want applications like ASP.NET to be able to hand out instances of a DataProtector
        // To do this, we demand unrestricted DataProtectionPermission in the constructor, but Assert
        // the permission when the work is actually done.  This makes the class behave similar to FileStream
        // where access is checked at time of creation, not time of use.
        [SecuritySafeCritical]
        [DataProtectionPermission(SecurityAction.Assert, UnprotectData = true)]
        protected override byte[] ProviderUnprotect(byte[] encryptedData)
        {
            // Delegate to ProtectedData
            return ProtectedData.Unprotect(encryptedData, GetHashedPurpose(), Scope);
        }

        public override bool IsReprotectRequired(byte[] encryptedData)
        {
            // For now, assume we can't determine this for DPAPI
            return true;
        }

        // Public constructor
        // NOTE: This Demand must be here because we are going to Assert this permission in the ProviderProtect/ProviderUnprotect
        // methods.  See comments above for explanation.
        [DataProtectionPermission(SecurityAction.Demand, Unrestricted = true)]
        [SecuritySafeCritical]
        public DpapiDataProtector(string appName,
                                  string primaryPurpose,
                                  params string[] specificPurpose)
            : base(appName, primaryPurpose, specificPurpose)
        {
        }
    }
}
