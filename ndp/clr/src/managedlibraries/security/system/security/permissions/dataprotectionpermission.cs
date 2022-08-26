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
    using System.Globalization;

    [Serializable()]
    public sealed class DataProtectionPermission : CodeAccessPermission, IUnrestrictedPermission {
        private DataProtectionPermissionFlags m_flags;

        public DataProtectionPermission (PermissionState state) {
            if (state == PermissionState.Unrestricted)
                m_flags = DataProtectionPermissionFlags.AllFlags;
            else if (state == PermissionState.None)
                m_flags = DataProtectionPermissionFlags.NoFlags;
            else
                throw new ArgumentException(SecurityResources.GetResourceString("Argument_InvalidPermissionState"));
        }

        public DataProtectionPermission (DataProtectionPermissionFlags flag) {
            this.Flags = flag;
        }

        public DataProtectionPermissionFlags Flags {
            set {
                VerifyFlags(value);
                m_flags = value;
            }
            get {
                return m_flags;
            }
        }

        //
        // IUnrestrictedPermission implementation
        //

        public bool IsUnrestricted ()  {
            return m_flags == DataProtectionPermissionFlags.AllFlags;
        }

        //
        // IPermission implementation
        //

        public override IPermission Union (IPermission target) {
            if (target == null)
                return this.Copy();

            try {
                DataProtectionPermission operand = (DataProtectionPermission) target;
                DataProtectionPermissionFlags flag_union = m_flags | operand.m_flags;
                if (flag_union == DataProtectionPermissionFlags.NoFlags)
                    return null;
                else
                    return new DataProtectionPermission(flag_union);
            } 
            catch (InvalidCastException) {
                throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }
        }

        public override bool IsSubsetOf (IPermission target) {
            if (target == null) 
                return m_flags == DataProtectionPermissionFlags.NoFlags;

            try {
                DataProtectionPermission operand = (DataProtectionPermission) target;
                DataProtectionPermissionFlags sourceFlag = this.m_flags;
                DataProtectionPermissionFlags targetFlag = operand.m_flags;
                return ((sourceFlag & targetFlag) == sourceFlag);
            } 
            catch (InvalidCastException) {
                throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }
        }

        public override IPermission Intersect (IPermission target) {
            if (target == null)
                return null;

            try {
                DataProtectionPermission operand = (DataProtectionPermission) target;
                DataProtectionPermissionFlags flag_intersect = operand.m_flags & this.m_flags;
                if (flag_intersect == DataProtectionPermissionFlags.NoFlags)
                    return null;
                else
                    return new DataProtectionPermission(flag_intersect);
            }
            catch (InvalidCastException) {
                throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Argument_WrongType"), this.GetType().FullName));
            }
        }

        public override IPermission Copy () {
            if (this.Flags == DataProtectionPermissionFlags.NoFlags)
                return null;
            return new DataProtectionPermission(m_flags);
        }

        //
        // FromXml/ToXml
        //

        public override SecurityElement ToXml () {
            SecurityElement securityElement = new SecurityElement("IPermission");
            securityElement.AddAttribute("class", this.GetType().FullName + ", " + this.GetType().Module.Assembly.FullName.Replace('\"', '\''));
            securityElement.AddAttribute("version", "1");
            if (!IsUnrestricted()) 
                securityElement.AddAttribute("Flags", m_flags.ToString());
            else 
                securityElement.AddAttribute("Unrestricted", "true");
            return securityElement;
        }

        public override void FromXml (SecurityElement securityElement) {
            if (securityElement == null)
                throw new ArgumentNullException("securityElement");

            string className = securityElement.Attribute("class");
            if (className == null || className.IndexOf(this.GetType().FullName, StringComparison.Ordinal) == -1)
                throw new ArgumentException(SecurityResources.GetResourceString("Argument_InvalidClassAttribute"), "securityElement");

            string unrestricted = securityElement.Attribute("Unrestricted");
            if (unrestricted != null && String.Compare(unrestricted, "true", StringComparison.OrdinalIgnoreCase) == 0) {
                m_flags = DataProtectionPermissionFlags.AllFlags;
                return;
            }

            m_flags = DataProtectionPermissionFlags.NoFlags;
            string strFlags = securityElement.Attribute("Flags");
            if (strFlags != null) {
                DataProtectionPermissionFlags flags = (DataProtectionPermissionFlags) Enum.Parse(typeof(DataProtectionPermissionFlags), strFlags);
                VerifyFlags(flags);
                m_flags = flags;
            }
        }

        internal static void VerifyFlags (DataProtectionPermissionFlags flags) {
            if ((flags & ~DataProtectionPermissionFlags.AllFlags) != 0)
                throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Arg_EnumIllegalVal"), (int)flags));
        }
    }
}
