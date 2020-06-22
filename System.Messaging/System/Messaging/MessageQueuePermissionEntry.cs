//----------------------------------------------------
// <copyright file="MessageQueuePermissionEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.ComponentModel;

    /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry"]/*' />
    [Serializable()]
    public class MessageQueuePermissionEntry
    {
        private string label;
        private string machineName;
        private string path;
        private string category;
        private MessageQueuePermissionAccess permissionAccess;

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.MessageQueuePermissionEntry"]/*' />
        public MessageQueuePermissionEntry(MessageQueuePermissionAccess permissionAccess, string path)
        {
            if (path == null)
                throw new ArgumentNullException("path");

            if (path != MessageQueuePermission.Any && !MessageQueue.ValidatePath(path, false))
                throw new ArgumentException(Res.GetString(Res.PathSyntax));

            this.path = path;

            this.permissionAccess = permissionAccess;
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.MessageQueuePermissionEntry1"]/*' />
        public MessageQueuePermissionEntry(MessageQueuePermissionAccess permissionAccess, string machineName, string label, string category)
        {
            if (machineName == null && label == null && category == null)
                throw new ArgumentNullException("machineName");

            if (machineName != null && !SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "MachineName", machineName));

            this.permissionAccess = permissionAccess;
            this.machineName = machineName;
            this.label = label;
            this.category = category;
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.Category"]/*' />
        public string Category
        {
            get
            {
                return this.category;
            }
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.Label"]/*' />
        public string Label
        {
            get
            {
                return this.label;
            }
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.MachineName"]/*' />
        public string MachineName
        {
            get
            {
                return this.machineName;
            }
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.Path"]/*' />
        public string Path
        {
            get
            {
                return this.path;
            }
        }

        /// <include file='doc\MessageQueuePermissionEntry.uex' path='docs/doc[@for="MessageQueuePermissionEntry.PermissionAccess"]/*' />
        public MessageQueuePermissionAccess PermissionAccess
        {
            get
            {
                return this.permissionAccess;
            }
        }
    }
}

