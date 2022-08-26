//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Security.AccessControl;

    // class used for poping up the ACL Window
    static class ACLWrapper
    {
        // wrapper for actual call to Win32 function that pops up the ACL window
        internal static bool EditACLSecurity(
            ISecurityInformationManaged model,
            IntPtr hwndOwner)
        {
            // create a new instance of a ISecurityInformation object
            ISecurityInformation psi = new SecurityInfoCCW(model);
            // pop-up the ACL window for the ISecurityInformation object created
#pragma warning suppress 56523
            int result = SafeNativeMethods.EditSecurity(hwndOwner, psi);
            return result == 0 ? false : true;
        }
    }

    // needed only for the PropertySheetPageCallback function
    // no actual use in this project
    enum PageType
    {
        SI_PAGE_PERM = 0,
        SI_PAGE_ADVPERM,
        SI_PAGE_AUDIT,
        SI_PAGE_OWNER
    };

    // Flags that are used in conjunction with the ObjectInformation
    enum ObjectInfoFlags : int
    {
        EditPerms = 0x00000000,
        EditOwner = 0x00000001,
        EditAudit = 0x00000002,
        EditAll = 0x00000003,
        Container = 0x00000004,
        ReadOnly = 0x00000008,
        Advanced = 0x00000010,
        Reset = 0x00000020,
        OwnerReadOnly = 0x00000040,
        EditProperties = 0x00000080,
        OwnerRecurse = 0x00000100,
        NoAclProtect = 0x00000200,
        NoTreeApply = 0x00000400,
        PageTitle = 0x00000800,
        ServerIsDC = 0x00001000,
        ResetDaclTree = 0x00004000,
        ResetSaclTree = 0x00008000,
        ObjectGuid = 0x00010000,
        EditEffective = 0x00020000,
        ResetDacl = 0x00040000,
        ResetSacl = 0x00080000,
        ResetOwner = 0x00100000,
        NoAdditionalPermission = 0x00200000,
        MayWrite = 0x10000000
    };

    // ObjectInformation defines the content and available resources 
    // within the ACL window
    [StructLayout(LayoutKind.Sequential)]
    struct ObjectInfo
    {
        internal ObjectInfoFlags Flags;
        internal IntPtr hInstance; // must be IntPtr.Zero or a SafeHandle must be used
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string ServerName;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string ObjectName;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string PageTitle;
        internal Guid ObjectType; // ignored unless ObjectInfoFlags.ObjectGuid is set

        internal ObjectInfo(
            ObjectInfoFlags flags,
            string serverName,
            string objectName,
            string pageTitle)
        {
            this.Flags = flags;
            this.ServerName = serverName;
            this.ObjectName = objectName;
            this.PageTitle = pageTitle;
            this.ObjectType = Guid.Empty;
            this.hInstance = IntPtr.Zero;
            if (this.PageTitle != null)
            {
                this.Flags = this.Flags | ObjectInfoFlags.PageTitle;
            }
        }
    };

    // AccessFlags tell the ACL window where to put Access properties 
    enum AccessFlags : int
    {
        Specific = 0x00010000,
        General = 0x00020000,
        Container = 0x00040000,
        Property = 0x00080000
    };


    // Structure that defines each property
    [StructLayout(LayoutKind.Sequential)]
    struct Access
    {
        internal IntPtr Guid; // leave this empty unless you are using Active Directory style ACLs
        internal int Mask;
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Name;
        internal AccessFlags Flags;

        internal Access(
            int mask,
            String name,
            AccessFlags flags)
        {
            this.Guid = IntPtr.Zero;
            this.Mask = mask;
            this.Name = name;
            this.Flags = flags;
        }
    };

    // Inheritance flags are defined only for the GetInheritTypes function
    // this is not used in this project
    enum InheritFlags : byte
    {
        Object = 1,
        Container = 2,
        InheritOnly = 8
    };

    // Inheritance Type is defined only for the GetInheritTypes function
    // this is not used in this project
    [StructLayout(LayoutKind.Sequential)]
    struct InheritType
    {
        internal Guid Guid;
        internal InheritFlags Flags;
        internal String Name;
    };

    enum AceFlags : byte
    {
        ObjectInherit = 0x01,
        ContainerInherit = 0x02,
        NoPropagateInherit = 0x04,
        InheritOnly = 0x08,
        Inherited = 0x10,
        SuccessAudit = 0x40,
        FailureAudit = 0x80
    };

    // Holds togather all the permissions for an object
    class AccessRightsWrapper
    {
        internal Access[] access;
        internal int DefaultIndex;  // indicates the item in Access array that
        // should be used for default rights for new objects
    };

    [StructLayout(LayoutKind.Sequential)]
    struct GenericAccess
    {
        internal Guid ObjectType;
        internal AceFlags AceFlags;
        internal int Mask;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct GenericMapping
    {
        internal int GenericRead;
        internal int GenericWrite;
        internal int GenericExecute;
        internal int GenericAll;

        internal GenericMapping(
            int read,
            int write,
            int execute,
            int all)
        {
            GenericRead = read;
            GenericWrite = write;
            GenericExecute = execute;
            GenericAll = all;
        }
    };

    // interface for the WSATSecurityModel
    interface ISecurityInformationManaged
    {
        ObjectInfo GetObjectInformation();

        AccessRightsWrapper GetAccessRights(
            Guid objectType,
            ObjectInfoFlags flags);

        IntPtr GetSecurity(
            SecurityInfos requestedInformation,
            bool wantDefault);

        void SetSecurity(
            SecurityInfos providedInformation,
            IntPtr pSecurityDescriptor);

        void MapGeneric(
            ref GenericAccess generic);

        InheritType[] GetInheritTypes();
    }

    // COM imported interface, ISecurityInformation
    [ComImport(), InterfaceType(ComInterfaceType.InterfaceIsIUnknown),
    Guid("965FC360-16FF-11d0-91CB-00AA00BBB723")]
    interface ISecurityInformation
    {
        void GetObjectInformation(
            out ObjectInfo pObjectInfo);

        void GetSecurity(
            SecurityInfos requestedInformation,
            out IntPtr ppSecurityDescriptor, // No need to use SafeHandle since the secDesc memory will be allocated and freed by system
            bool fDefault);

        void SetSecurity(
            SecurityInfos securityInformation,
            IntPtr pSecurityDescriptor);

        void GetAccessRights(
            ref Guid pguidObjectType,
            ObjectInfoFlags flags,
            [MarshalAs(UnmanagedType.LPArray)] out Access[] ppAccess,
            out int pcAccesses,
            out int piDefaultAccess);

        void MapGeneric(
            ref Guid pguidObjectType,
            ref AceFlags pAceFlags,
            ref int pMask);

        void GetInheritTypes(
            out InheritType[] ppInheritTypes,
            out uint count);

        void PropertySheetPageCallback(
            IntPtr hwnd,
            int uMsg,
            PageType uPage);
    }

    [ComVisible(true), Guid("8aa377ab-464b-44c8-9b02-5a7c09bbf18a")]
    struct SecurityInfoCCW : ISecurityInformation
    {
        ISecurityInformationManaged model;

        public SecurityInfoCCW(ISecurityInformationManaged model)
        {
            this.model = model;
        }
        
        public void GetObjectInformation(out ObjectInfo pObjectInfo)
        {
            pObjectInfo = model.GetObjectInformation();
        }

        public void GetSecurity(
            SecurityInfos requestedInformation,
            out IntPtr ppSecurityDescriptor,
            bool fDefault)
        {
            ppSecurityDescriptor = model.GetSecurity(requestedInformation, fDefault);
        }

        public void SetSecurity(
            SecurityInfos securityInformation,
            IntPtr pSecurityDescriptorFromUI)
        {
            // SecurityInformation is a enumeration - we don't need to check that if being null
            // pSecurityDescriptorFromUI is the security descriptor that we receive from the UI 
            // and need to store in the registry.
            if (pSecurityDescriptorFromUI.Equals(IntPtr.Zero))
            {
                throw new ArgumentNullException(SR.GetString(SR.ACLEditorSetSecurityError));
            }
            model.SetSecurity(
                securityInformation,
                pSecurityDescriptorFromUI);
        }

        public void GetAccessRights(
            ref Guid pguidObjectType,
            ObjectInfoFlags dwFlags,
            [MarshalAs(UnmanagedType.LPArray)] out Access[] ppAccess,
            out int pcAccesses,
            out int piDefaultAccess)
        {
            // should set objectType to the pguidObjectType, but we don't need that
            // => we leave objectType as Guid.Empty
            Guid objectType = Guid.Empty;

            AccessRightsWrapper accessRights = model.GetAccessRights(objectType, dwFlags);
            Access[] access = accessRights.access;

            ppAccess = access;
            pcAccesses = (int)access.Length;
            piDefaultAccess = (int)accessRights.DefaultIndex;
        }

        public void MapGeneric(
            ref Guid pguidObjectType,
            ref AceFlags pAceFlags,
            ref int pMask)
        {
            if (pAceFlags == 0 || pMask == 0) return;
            GenericAccess ga = new GenericAccess();
            ga.AceFlags = pAceFlags;
            ga.Mask = pMask;
            this.model.MapGeneric(ref ga);
            pAceFlags = ga.AceFlags;
            pMask = ga.Mask;
        }

        // this method is never invoked! it is here only due to the interface ISecurityInformation
        public void GetInheritTypes(
            out InheritType[] ppInheritTypes,
            out uint count)
        {
            // not implemented. it is never used!
            ppInheritTypes = new InheritType[0];
            count = 0;
        }

        public void PropertySheetPageCallback(
            IntPtr hwnd,
            int uMsg,
            PageType uPage)
        {
            // no implemetation required here
        }
    }
}
