using System;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Globalization;

namespace System.Transactions
{
    [SerializableAttribute()]
    public sealed class DistributedTransactionPermission : CodeAccessPermission, IUnrestrictedPermission
    {  
        private bool unrestricted;

        public DistributedTransactionPermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
            {
                unrestricted = true;
            }
            else
            {
                unrestricted = false;
            }
        }     

        public bool IsUnrestricted()
        {
            return unrestricted;
        }

        //Define the rest of your custom permission here. You must 
        //implement IsUnrestricted, and override Copy, Intersect, 
        //IsSubsetOf, ToXML, and FromXML.

        public override IPermission Copy()
        {
            DistributedTransactionPermission copy = new DistributedTransactionPermission(PermissionState.None);

            if (this.IsUnrestricted())
            {
                copy.unrestricted = true;
            }
            else
            {
                copy.unrestricted = false;
            }
            return copy;
        }

        public override IPermission Intersect(IPermission target)
        {
            try
            {
                if (null == target)
                {
                    return null;
                }
                DistributedTransactionPermission passedPermission = (DistributedTransactionPermission)target;

                if (!passedPermission.IsUnrestricted())
                {
                    return passedPermission;
                }
                return this.Copy();
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException( SR.GetString( SR.ArgumentWrongType ), "target");
            }                
        }

        public override IPermission Union(IPermission target)
        {
            try
            {
                if (null == target)
                {
                    return this.Copy();
                }
                DistributedTransactionPermission passedPermission = (DistributedTransactionPermission)target;

                if (passedPermission.IsUnrestricted())
                {
                    return passedPermission;
                }

                return this.Copy();
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException( SR.GetString( SR.ArgumentWrongType ), "target");
            }                
        }

        public override bool IsSubsetOf(IPermission target)
        {  
            if (null == target)
            {
                return !this.unrestricted;
            }
            try
            {        
                DistributedTransactionPermission passedPermission = (DistributedTransactionPermission)target;
                // If the current permission is "none" it will always be a subset of
                // any other DistributedTransactionPermission.
                if ( !this.unrestricted )
                {
                    return true;
                }

                // At this point, we know the current permission is unrestricted.  If
                // the passed permission is unrestricted, we have a subset.
                if ( passedPermission.unrestricted )
                {
                    return true;
                }

                // If we get here, the current permission is unrestricted, but the passed
                // permission is "none", so the current permission is NOT a subset of
                // the passed permission.
                return false;
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException( SR.GetString( SR.ArgumentWrongType ), "target");
            }                
        }

        public override SecurityElement ToXml()
        {
            SecurityElement element = new SecurityElement("IPermission");
            Type type = this.GetType();
            StringBuilder assemblyName = new StringBuilder(type.Assembly.ToString());
            assemblyName.Replace('\"', '\'');
            element.AddAttribute("class", type.FullName + ", " + assemblyName);
            element.AddAttribute("version", "1");
            element.AddAttribute("Unrestricted", unrestricted.ToString());
            return element;
        }

        public override void FromXml(SecurityElement securityElement)
        {
            if ( null == securityElement )
            {
                throw new ArgumentNullException( "securityElement" );
            }

            if (!securityElement.Tag.Equals ("IPermission"))
            {
                throw new ArgumentException( SR.GetString( SR.ArgumentWrongType ), "securityElement" );
            }

            string element = securityElement.Attribute("Unrestricted");
            if (null != element)
            {  
                this.unrestricted = Convert.ToBoolean(element, CultureInfo.InvariantCulture);
            }
            else
            {
                this.unrestricted = false;
            }
        }

    }

    [AttributeUsageAttribute(AttributeTargets.All, AllowMultiple = true)]
    public sealed class DistributedTransactionPermissionAttribute : CodeAccessSecurityAttribute
    {
        bool unrestricted = false;

        public new bool Unrestricted
        {
            get { return unrestricted; }
            set { unrestricted = value; }
        }

        public DistributedTransactionPermissionAttribute(SecurityAction action) : base (action)
        {  
        }
        public override IPermission CreatePermission()
        {
            if (Unrestricted)
            {
                return new DistributedTransactionPermission(PermissionState.Unrestricted);
            }
            else
            {
                return new DistributedTransactionPermission(PermissionState.None);
            }
        }
    }
}
