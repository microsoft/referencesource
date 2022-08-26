//------------------------------------------------------------------------------
// <copyright file="MessageQueuePermissionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.ComponentModel;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly | AttributeTargets.Event, AllowMultiple = true, Inherited = false),
    Serializable()]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1813:AvoidUnsealedAttributes")]
    public class MessageQueuePermissionAttribute : CodeAccessSecurityAttribute
    {
        private string label;
        private string machineName;
        private string path;
        private string category;
        private MessageQueuePermissionAccess permissionAccess;

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.MessageQueuePermissionAttribute"]/*' />
        public MessageQueuePermissionAttribute(SecurityAction action)
            : base(action)
        {
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.Category"]/*' />
        public string Category
        {
            get
            {
                return this.category;
            }

            set
            {
                string oldValue = this.category;
                this.category = value;
                Exception e = CheckProperties();
                if (e != null)
                {
                    this.category = oldValue;
                    throw e;
                }
            }
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.Label"]/*' />
        public string Label
        {
            get
            {
                return this.label;
            }

            set
            {
                string oldValue = this.label;
                this.label = value;
                Exception e = CheckProperties();
                if (e != null)
                {
                    this.label = oldValue;
                    throw e;
                }
            }
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.MachineName"]/*' />
        public string MachineName
        {
            get
            {
                return this.machineName;
            }

            set
            {
                if (value != null && !SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "MachineName", value));

                string oldValue = this.machineName;
                this.machineName = value;
                Exception e = CheckProperties();
                if (e != null)
                {
                    this.machineName = oldValue;
                    throw e;
                }
            }
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.Path"]/*' />
        public string Path
        {
            get
            {
                return this.path;
            }

            set
            {
                if (value != null && value != MessageQueuePermission.Any && !MessageQueue.ValidatePath(value, false))
                    throw new ArgumentException(Res.GetString(Res.PathSyntax));

                string oldValue = this.path;
                this.path = value;
                Exception e = CheckProperties();
                if (e != null)
                {
                    this.path = oldValue;
                    throw e;
                }
            }
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.PermissionAccess"]/*' />
        public MessageQueuePermissionAccess PermissionAccess
        {
            get
            {
                return this.permissionAccess;
            }

            set
            {
                this.permissionAccess = value;
            }
        }

        /// <include file='doc\MessageQueuePermissionAttribute.uex' path='docs/doc[@for="MessageQueuePermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission()
        {
            if (Unrestricted)
                return new MessageQueuePermission(PermissionState.Unrestricted);

            CheckProperties();
            if (this.path != null)
                return new MessageQueuePermission(this.PermissionAccess, this.path);

            return new MessageQueuePermission(this.PermissionAccess, this.machineName, this.label, this.category);
        }

        private Exception CheckProperties()
        {
            if (this.path != null &&
                (this.machineName != null || this.label != null || this.category != null))
                return new InvalidOperationException(Res.GetString(Res.PermissionPathOrCriteria));

            if (this.path == null &&
                this.machineName == null && this.label == null && this.category == null)
                return new InvalidOperationException(Res.GetString(Res.PermissionAllNull));

            return null;
        }

    }
}

