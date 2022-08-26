// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================

using System.Runtime.InteropServices;
using System.Text;

namespace System.Runtime.Remoting.Channels{ 
    
    internal static class NativeMethods {    
        private const string ADVAPI32 = "advapi32.dll";                                                 
        
        [DllImport(ADVAPI32, SetLastError=true)]
        internal static extern bool IsValidSid(IntPtr sidPointer);
                                                    
        [DllImport(ADVAPI32, SetLastError=true)]
        internal static extern IntPtr GetSidIdentifierAuthority(IntPtr sidPointer);
    
        [DllImport(ADVAPI32, SetLastError=true)]
        internal static extern IntPtr GetSidSubAuthorityCount(IntPtr sidPointer);
        
        [DllImport(ADVAPI32, SetLastError=true)]
        internal static extern IntPtr GetSidSubAuthority(IntPtr sidPointer, int count);
    
        [DllImport(ADVAPI32, SetLastError=true)]
        internal static extern bool GetTokenInformation(IntPtr tokenHandle, int tokenInformationClass, IntPtr sidAndAttributesPointer, 
                                                                             int tokenInformationLength, ref int returnLength);                                                                             
                                                                                                                                              
        internal const int ThreadTokenAllAccess  = 0x000F0000 | 0x01FF;
        internal const int BufferTooSmall = 0x7A;
        internal enum TokenInformationClass {
            TokenUser = 1, 
            TokenGroups, 
            TokenPrivileges, 
            TokenOwner, 
            TokenPrimaryGroup, 
            TokenDefaultDacl, 
            TokenSource, 
            TokenType, 
            TokenImpersonationLevel, 
            TokenStatistics, 
            TokenRestrictedSids, 
            TokenSessionId, 
            TokenGroupsAndPrivileges, 
            TokenSessionReference, 
            TokenSandBoxInert
        }             
            
    }
}
