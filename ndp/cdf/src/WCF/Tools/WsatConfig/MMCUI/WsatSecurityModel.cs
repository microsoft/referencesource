//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Security.AccessControl;
    using System.Security.Authentication;
    using System.Security.Cryptography;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Security.Principal;

    class WsatSecurityModel : ISecurityInformationManaged
    {
        WsatConfiguration current;

        // the name of the machine that will do account-SID-account conversions
        string machineName;
        // prop-sheet header
        string objectName;
        // porp-sheet title
        string pageTitle;

        // the object specifying data regarding object in the ACL
        static ObjectInfo objectInformation;

        // the access right for the ACL control
        static AccessRightsWrapper accessRights;

        // we declare these constants for mapping with the generic 
        // r,w,x,a permissions
        // LC SW RP == 0x0000001C
        const int WSAT_ALL = 0x0000001C;

        // GenericMapping structure holds the actual r,w,x,a constants
        GenericMapping ServiceGenericMapping = new GenericMapping(
            WSAT_ALL,
            WSAT_ALL,
            WSAT_ALL,
            WSAT_ALL);

        public WsatSecurityModel(string machineName, WsatConfiguration current)
        {
            this.current = current;
            this.machineName = machineName;
            this.objectName = SR.GetString(SR.ACLEditorPageTitle);
            this.pageTitle = SR.GetString(SR.ACLEditorObjectName);

            // we only want to edit permissions (this is the default value anyway)
            objectInformation = new ObjectInfo(
                ObjectInfoFlags.EditPerms | ObjectInfoFlags.Container,
                this.machineName,
                this.objectName,
                this.pageTitle
            );

            if (accessRights == null)
            {
                accessRights = new AccessRightsWrapper();
                accessRights.access = new Access[1];
                // summary page permissions --- AccessFlags.General
                accessRights.access[0] = new Access(WSAT_ALL, SR.GetString(SR.ACLEditorPermissionName), AccessFlags.General);
                // this is de default mask when adding a new ACE --- WSAT_ALL
                accessRights.DefaultIndex = 0;
            }
        }

        public ObjectInfo GetObjectInformation()
        {
            return objectInformation;
        }

        // define the access rights
        // we do not use the GUID (it is usefull only when using AD style objects
        public AccessRightsWrapper GetAccessRights(Guid objectType, ObjectInfoFlags flags)
        {
            return accessRights;
        }

        public InheritType[] GetInheritTypes()
        {
            // WSAT permissions have no inheritance
            return null;
        }

        public void MapGeneric(ref GenericAccess generic)
        {
            SafeNativeMethods.MapGenericMask(out generic.Mask, ref ServiceGenericMapping);
        }

        // this method is called by SecurityInfoCCW.SetSecurity in order to handle 
        // security information saving process
        // it stores the data into the registry
        // [saves data from UI to registry]
        public void SetSecurity(
            SecurityInfos providedInformation,
            IntPtr pSecurityDescriptor
            )
        {
            string stringSecurityDescriptor = "";
            SecurityIdentifier sid;

            IntPtr pszSD;
            int size = 0;

#pragma warning suppress 56523
            bool ret = SafeNativeMethods.ConvertSecurityDescriptorToStringSecurityDescriptorW(
                pSecurityDescriptor,
                1 /* SDDL_REVISION_1 == 1 (according to specs, this should always be 1 */,
                providedInformation,
                out pszSD,
                out size
                );
            if (!ret)
            {
                current.KerberosGlobalAcl = new string[] { "" };
                return;
            }

            stringSecurityDescriptor = Marshal.PtrToStringUni(pszSD);
#pragma warning suppress 56523
            SafeNativeMethods.LocalFree(pszSD);

            ArrayList allowed = new ArrayList();
            RawAcl rawDacl = new RawSecurityDescriptor(stringSecurityDescriptor).DiscretionaryAcl;
            DiscretionaryAcl dacl = new DiscretionaryAcl(false, false, rawDacl);

            for (int i = 0; i < dacl.Count; i++)
            {
                if (((CommonAce)dacl[i]).AceType == AceType.AccessAllowed)
                {
                    sid = ((CommonAce)dacl[i]).SecurityIdentifier;
                    allowed.Add(sid.Translate(typeof(NTAccount)).Value);
                }
            }

            current.KerberosGlobalAcl = (string[])allowed.ToArray(typeof(string));
        }

        // this method is called by SecurityInfoCCW.GetSecurity  
        // its return is the SecurityDescriptor in binary format
        // it loads the data from the registry
        // [loads data from registry to UI]
        public IntPtr GetSecurity(SecurityInfos requestedInformation, bool wantDefault)
        {
            if (requestedInformation == SecurityInfos.DiscretionaryAcl)
            {
                StringBuilder securityDescriptorBuilder = new StringBuilder("D:");

                System.Collections.ArrayList kerb = new System.Collections.ArrayList(current.KerberosGlobalAcl);
                System.Collections.ArrayList indexesOfInvalidItems = new System.Collections.ArrayList();
                for (int i = 0; i < kerb.Count; i++)
                {
                    try
                    {
                        string sid = ((new NTAccount((string)kerb[i])).Translate(typeof(SecurityIdentifier))).ToString();
                        securityDescriptorBuilder.Append("(A;;LCSWRP;;;" + sid + ")");
                    }
                    catch (ArgumentException) // invalid account, do not consider it
                    {
                        indexesOfInvalidItems.Add(i);
                    }
                    catch (IdentityNotMappedException)
                    {
                        indexesOfInvalidItems.Add(i);
                    }
                }

                //remove invalid items based on indexesOfInvalidItems
                for (int i = indexesOfInvalidItems.Count - 1; i >= 0; i--)
                {
                    kerb.RemoveAt((int)indexesOfInvalidItems[i]);
                }

                // rebuild the ACL, taking care not to leave it null
                if (kerb.Count <= 0)
                {
                    current.KerberosGlobalAcl = new string[] { "" };
                }
                else
                {
                    current.KerberosGlobalAcl = (string[])kerb.ToArray(typeof(string));
                }

                IntPtr securityDescriptor;
                int size = 0;

                // call external function for transformig String SecurityDescriptors
                // into their internal representation
#pragma warning suppress 56523
                bool ret = SafeNativeMethods.ConvertStringSecurityDescriptorToSecurityDescriptor(
                    securityDescriptorBuilder.ToString(),
                    1, /* 
                        * must be SDDL_REVISION_1 == 1 always
                        */
                    out securityDescriptor,
                    out size
                );
                if (!ret)
                {
                    return IntPtr.Zero;
                }
                return securityDescriptor;
            }
            return IntPtr.Zero;
        }
    }
}
