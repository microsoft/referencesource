//----------------------------------------------------
// <copyright file="MessageQueuePermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;


    /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable()]
    public sealed class MessageQueuePermission : CodeAccessPermission, IUnrestrictedPermission
    {
        internal Hashtable resolvedFormatNames;
        internal MessageQueuePermissionEntryCollection innerCollection;
        internal bool isUnrestricted;
        internal const string Any = "*";

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermissionEntry.MessageQueuePermission"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermission()
        {
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.MessageQueuePermission1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermission(PermissionState state)
        {
            if (state == PermissionState.Unrestricted)
                this.isUnrestricted = true;
            else
                this.isUnrestricted = false;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.MessageQueuePermission2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermission(MessageQueuePermissionAccess permissionAccess, string path)
        {
            MessageQueuePermissionEntry entry = new MessageQueuePermissionEntry(permissionAccess, path);
            this.PermissionEntries.Add(entry);
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.MessageQueuePermission3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermission(MessageQueuePermissionAccess permissionAccess, string machineName, string label, string category)
        {
            MessageQueuePermissionEntry entry = new MessageQueuePermissionEntry(permissionAccess, machineName, label, category);
            this.PermissionEntries.Add(entry);
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.MessageQueuePermission4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermission(MessageQueuePermissionEntry[] permissionAccessEntries)
        {
            if (permissionAccessEntries == null)
                throw new ArgumentNullException("permissionAccessEntries");

            this.PermissionEntries.AddRange(permissionAccessEntries);
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.PermissionEntries"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueuePermissionEntryCollection PermissionEntries
        {
            get
            {
                if (this.innerCollection == null)
                {
                    if (this.resolvedFormatNames == null)
                        this.innerCollection = new MessageQueuePermissionEntryCollection(this);
                    else
                    {
                        Hashtable resolvedReference = this.resolvedFormatNames;
                        this.innerCollection = new MessageQueuePermissionEntryCollection(this);
                        foreach (string formatName in resolvedReference.Keys)
                        {
                            string path;
                            if (formatName == Any)
                                path = Any;
                            else
                                path = "FORMATNAME:" + formatName;

                            MessageQueuePermissionEntry entry = new MessageQueuePermissionEntry(
                                                                                        (MessageQueuePermissionAccess)resolvedReference[formatName],
                                                                                         path);
                            this.innerCollection.Add(entry);
                        }
                    }
                }

                return this.innerCollection;
            }
        }

        // Put this in one central place.  MSMQ appears to use CompareString
        // with LOCALE_INVARIANT and NORM_IGNORECASE.
        private static IEqualityComparer GetComparer()
        {
            return StringComparer.InvariantCultureIgnoreCase;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.Clear"]/*' />
        /// <internalonly/> 
        internal void Clear()
        {
            this.resolvedFormatNames = null;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.Copy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Copy()
        {
            MessageQueuePermission permission = new MessageQueuePermission();
            permission.isUnrestricted = this.isUnrestricted;
            foreach (MessageQueuePermissionEntry entry in this.PermissionEntries)
                permission.PermissionEntries.Add(entry);

            permission.resolvedFormatNames = this.resolvedFormatNames;
            return permission;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.FromXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1803:AvoidCostlyCallsWherePossible")]
        public override void FromXml(SecurityElement securityElement)
        {
            this.PermissionEntries.Clear();
            string unrestrictedValue = securityElement.Attribute("Unrestricted");
            if (unrestrictedValue != null && (string.Compare(unrestrictedValue, "true", true, CultureInfo.InvariantCulture) == 0))
            {
                this.isUnrestricted = true;
                return;
            }

            if (securityElement.Children != null)
            {
                for (int index = 0; index < securityElement.Children.Count; ++index)
                {
                    SecurityElement currentElement = (SecurityElement)securityElement.Children[index];
                    MessageQueuePermissionEntry entry = null;

                    string accessString = currentElement.Attribute("access");
                    int permissionAccess = 0;
                    if (accessString != null)
                    {
                        string[] accessArray = accessString.Split(new char[] { '|' });
                        for (int index2 = 0; index2 < accessArray.Length; ++index2)
                        {
                            string currentAccess = accessArray[index2].Trim();
                            if (Enum.IsDefined(typeof(MessageQueuePermissionAccess), currentAccess))
                                permissionAccess = permissionAccess | (int)Enum.Parse(typeof(MessageQueuePermissionAccess), currentAccess);
                        }
                    }

                    if (currentElement.Tag == "Path")
                    {
                        string path = currentElement.Attribute("value");
                        if (path == null)
                            throw new InvalidOperationException(Res.GetString(Res.InvalidXmlFormat));

                        entry = new MessageQueuePermissionEntry((MessageQueuePermissionAccess)permissionAccess, path);
                    }
                    else if (currentElement.Tag == "Criteria")
                    {
                        string label = currentElement.Attribute("label");
                        string category = currentElement.Attribute("category");
                        string machineName = currentElement.Attribute("machine");
                        if (machineName == null && label == null && category == null)
                            throw new InvalidOperationException(Res.GetString(Res.InvalidXmlFormat));

                        entry = new MessageQueuePermissionEntry((MessageQueuePermissionAccess)permissionAccess, machineName, label, category);
                    }
                    else
                        throw new InvalidOperationException(Res.GetString(Res.InvalidXmlFormat));

                    this.PermissionEntries.Add(entry);
                }
            }
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.Intersect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Intersect(IPermission target)
        {
            if (target == null)
                return null;

            if (!(target is MessageQueuePermission))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "target", target.ToString()));

            MessageQueuePermission targetQueuePermission = (MessageQueuePermission)target;
            if (this.IsUnrestricted())
                return targetQueuePermission.Copy();

            if (targetQueuePermission.IsUnrestricted())
                return this.Copy();

            this.ResolveFormatNames();
            targetQueuePermission.ResolveFormatNames();
            MessageQueuePermission newPermission = new MessageQueuePermission();
            Hashtable newFormatNames = new Hashtable(GetComparer());
            newPermission.resolvedFormatNames = newFormatNames;
            IDictionaryEnumerator formatNamesEnumerator;
            Hashtable formatNamesTable;
            if (this.resolvedFormatNames.Count < targetQueuePermission.resolvedFormatNames.Count)
            {
                formatNamesEnumerator = this.resolvedFormatNames.GetEnumerator();
                formatNamesTable = targetQueuePermission.resolvedFormatNames;
            }
            else
            {
                formatNamesEnumerator = targetQueuePermission.resolvedFormatNames.GetEnumerator();
                formatNamesTable = this.resolvedFormatNames;
            }

            while (formatNamesEnumerator.MoveNext())
            {
                if (formatNamesTable.ContainsKey(formatNamesEnumerator.Key))
                {
                    string currentFormatName = (string)formatNamesEnumerator.Key;
                    MessageQueuePermissionAccess currentAccess = (MessageQueuePermissionAccess)formatNamesEnumerator.Value;
                    MessageQueuePermissionAccess targetAccess = (MessageQueuePermissionAccess)formatNamesTable[currentFormatName];
                    newFormatNames.Add(currentFormatName, currentAccess & targetAccess);
                }
            }

            return newPermission;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.IsSubsetOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsSubsetOf(IPermission target)
        {
            if (target == null)
                return false;

            if (!(target is MessageQueuePermission))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "target", target.ToString()));

            MessageQueuePermission targetQueuePermission = (MessageQueuePermission)target;
            if (targetQueuePermission.IsUnrestricted())
                return true;
            else if (this.IsUnrestricted())
                return false;

            this.ResolveFormatNames();
            targetQueuePermission.ResolveFormatNames();

            //If one of the tables is empty the subset cannot be resolved reliably, should assume
            //then that they are not subset of each other.
            if ((this.resolvedFormatNames.Count == 0 && targetQueuePermission.resolvedFormatNames.Count != 0) ||
                (this.resolvedFormatNames.Count != 0 && targetQueuePermission.resolvedFormatNames.Count == 0))
                return false;

            //If the target table contains a wild card, all the current formatName access need to be
            //a subset of the target.                      
            IDictionaryEnumerator formatNamesEnumerator;
            if (targetQueuePermission.resolvedFormatNames.ContainsKey(Any))
            {
                MessageQueuePermissionAccess targetAccess = (MessageQueuePermissionAccess)targetQueuePermission.resolvedFormatNames[Any];
                formatNamesEnumerator = this.resolvedFormatNames.GetEnumerator();
                while (formatNamesEnumerator.MoveNext())
                {
                    MessageQueuePermissionAccess currentAccess = (MessageQueuePermissionAccess)formatNamesEnumerator.Value;
                    if ((currentAccess & targetAccess) != currentAccess)
                        return false;
                }

                return true;
            }

            //If the current table contains a wild card it can be treated as any other format name.
            formatNamesEnumerator = this.resolvedFormatNames.GetEnumerator();
            while (formatNamesEnumerator.MoveNext())
            {
                string currentFormatName = (string)formatNamesEnumerator.Key;
                if (!targetQueuePermission.resolvedFormatNames.ContainsKey(currentFormatName))
                    return false;
                else
                {
                    MessageQueuePermissionAccess currentAccess = (MessageQueuePermissionAccess)formatNamesEnumerator.Value;
                    MessageQueuePermissionAccess targetAccess = (MessageQueuePermissionAccess)targetQueuePermission.resolvedFormatNames[currentFormatName];
                    if ((currentAccess & targetAccess) != currentAccess)
                        return false;
                }
            }

            return true;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.IsUnrestricted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsUnrestricted()
        {
            return this.isUnrestricted;
        }

        internal void ResolveFormatNames()
        {
            if (this.resolvedFormatNames == null)
            {
                this.resolvedFormatNames = new Hashtable(GetComparer());
                IEnumerator enumerator = this.PermissionEntries.GetEnumerator();
                while (enumerator.MoveNext())
                {
                    MessageQueuePermissionEntry entry = (MessageQueuePermissionEntry)enumerator.Current;
                    if (entry.Path != null)
                    {
                        if (entry.Path == Any)
                            this.resolvedFormatNames.Add(Any, entry.PermissionAccess);
                        else
                        {
                            try
                            {
                                MessageQueue queue = new MessageQueue(entry.Path);
                                this.resolvedFormatNames.Add(queue.FormatName, entry.PermissionAccess);
                            }
                            catch
                            {
                                //if the format name cannot be resolved, it won't be added to the list
                                //permissions won't be granted.
                            }
                        }
                    }
                    else
                    {
                        try
                        {
                            MessageQueueCriteria criteria = new MessageQueueCriteria();
                            if (entry.MachineName != null)
                                criteria.MachineName = entry.MachineName;

                            if (entry.Category != null)
                                criteria.Category = new Guid(entry.Category);

                            if (entry.Label != null)
                                criteria.Label = entry.Label;

                            IEnumerator messageQueues = MessageQueue.GetMessageQueueEnumerator(criteria, false);
                            while (messageQueues.MoveNext())
                            {
                                MessageQueue queue = (MessageQueue)messageQueues.Current;
                                this.resolvedFormatNames.Add(queue.FormatName, entry.PermissionAccess);
                            }
                        }
                        catch
                        {
                            //if the criteria cannot be resolved, nothing will be added to the list
                            //permissions won't be granted.
                        }
                    }
                }
            }
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.ToXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SecurityElement ToXml()
        {
            SecurityElement root = new SecurityElement("IPermission");
            Type type = this.GetType();
            root.AddAttribute("class", type.FullName + ", " + type.Module.Assembly.FullName.Replace('\"', '\''));
            root.AddAttribute("version", "1");

            if (this.isUnrestricted)
            {
                root.AddAttribute("Unrestricted", "true");
                return root;
            }

            IEnumerator enumerator = this.PermissionEntries.GetEnumerator();
            while (enumerator.MoveNext())
            {
                SecurityElement currentElement = null;
                MessageQueuePermissionEntry entry = (MessageQueuePermissionEntry)enumerator.Current;
                if (entry.Path != null)
                {
                    currentElement = new SecurityElement("Path");
                    currentElement.AddAttribute("value", entry.Path);
                }
                else
                {
                    currentElement = new SecurityElement("Criteria");
                    if (entry.MachineName != null)
                        currentElement.AddAttribute("machine", entry.MachineName);

                    if (entry.Category != null)
                        currentElement.AddAttribute("category", entry.Category);

                    if (entry.Label != null)
                        currentElement.AddAttribute("label", entry.Label);
                }

                int currentAccess = (int)entry.PermissionAccess;
                if (currentAccess != 0)
                {
                    StringBuilder accessStringBuilder = null;
                    int[] enumValues = (int[])Enum.GetValues(typeof(MessageQueuePermissionAccess));
                    Array.Sort(enumValues, InvariantComparer.Default);
                    for (int index = (enumValues.Length - 1); index >= 0; --index)
                    {
                        if (enumValues[index] != 0 && ((currentAccess & enumValues[index]) == enumValues[index]))
                        {
                            if (accessStringBuilder == null)
                                accessStringBuilder = new StringBuilder();
                            else
                                accessStringBuilder.Append("|");

                            accessStringBuilder.Append(Enum.GetName(typeof(MessageQueuePermissionAccess), enumValues[index]));
                            currentAccess = currentAccess & (enumValues[index] ^ enumValues[index]);
                        }
                    }

                    currentElement.AddAttribute("access", accessStringBuilder.ToString());
                }

                root.AddChild(currentElement);
            }

            return root;
        }

        /// <include file='doc\MessageQueuePermission.uex' path='docs/doc[@for="MessageQueuePermission.Union"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Union(IPermission target)
        {
            if (target == null)
                return this.Copy();

            if (!(target is MessageQueuePermission))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "target", target.ToString()));

            MessageQueuePermission targetQueuePermission = (MessageQueuePermission)target;
            MessageQueuePermission newPermission = new MessageQueuePermission();
            if (this.IsUnrestricted() || targetQueuePermission.IsUnrestricted())
            {
                newPermission.isUnrestricted = true;
                return newPermission;
            }

            Hashtable newFormatNames = new Hashtable(GetComparer());
            this.ResolveFormatNames();
            targetQueuePermission.ResolveFormatNames();

            IDictionaryEnumerator formatNamesEnumerator = this.resolvedFormatNames.GetEnumerator();
            IDictionaryEnumerator targetFormatNamesEnumerator = targetQueuePermission.resolvedFormatNames.GetEnumerator();
            while (formatNamesEnumerator.MoveNext())
                newFormatNames[(string)formatNamesEnumerator.Key] = formatNamesEnumerator.Value;

            while (targetFormatNamesEnumerator.MoveNext())
            {
                if (!newFormatNames.ContainsKey(targetFormatNamesEnumerator.Key))
                    newFormatNames[targetFormatNamesEnumerator.Key] = targetFormatNamesEnumerator.Value;
                else
                {
                    MessageQueuePermissionAccess currentAccess = (MessageQueuePermissionAccess)newFormatNames[targetFormatNamesEnumerator.Key];
                    newFormatNames[targetFormatNamesEnumerator.Key] = currentAccess | (MessageQueuePermissionAccess)targetFormatNamesEnumerator.Value;
                }
            }

            newPermission.resolvedFormatNames = newFormatNames;
            return newPermission;
        }
    }
}
