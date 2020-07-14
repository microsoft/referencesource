//------------------------------------------------------------------------------
// <copyright file="MessageQueueInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Messaging
{
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Configuration.Install;
    using System.Collections;
    using Microsoft.Win32;

    /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller"]/*' />
    /// <devdoc>
    ///    <para>Allows you to install and configure a queue that your 
    ///       application needs in order to run. This class is called by the installation
    ///       utility, installutil.exe, when installing a <see cref='System.Messaging.MessageQueue'/>
    ///       .</para>
    ///    <note type="rnotes">
    ///       Do we install the
    ///       backend queue resource or some MessageQueue object. I.e. is this creating a
    ///       new backend queue resource? Do we need to say anthing about checking for Path
    ///       existence?
    ///    </note>
    /// </devdoc>
    public class MessageQueueInstaller : ComponentInstaller
    {

        private bool authenticate = false;
        private short basePriority = (short)0;
        private Guid category = Guid.Empty;
        private System.Messaging.EncryptionRequired encryptionRequired = System.Messaging.EncryptionRequired.Optional;
        private string label = String.Empty;
        private long maximumJournalSize = UInt32.MaxValue;
        private long maximumQueueSize = UInt32.MaxValue;
        private string multicastAddress = String.Empty;
        private string path = String.Empty;
        private bool transactional = false;
        private bool useJournalQueue = false;
        private AccessControlList permissions = null;

        private UninstallAction uninstallAction = System.Configuration.Install.UninstallAction.Remove;

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.MessageQueueInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueueInstaller()
            : base()
        {
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.MessageQueueInstaller1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public MessageQueueInstaller(MessageQueue componentToCopy)
            : base()
        {
            InternalCopyFromComponent(componentToCopy);
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Authenticate"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the queue to be installed only accepts authenticated messages.</para>
        /// </devdoc>
        [DefaultValue(false)]
        public bool Authenticate
        {
            get
            {
                return authenticate;
            }
            set
            {
                authenticate = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.BasePriority"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the base priority used
        ///       to route a public queue's messages over the network.</para>
        /// </devdoc>
        [DefaultValue(0)]
        public short BasePriority
        {
            get
            {
                return basePriority;
            }
            set
            {
                basePriority = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Category"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates an implementation-specific queue type.</para>
        ///    <note type="rnotes">
        ///       Wording. Shorter
        ///       ("Indicates the queue's type") better here?
        ///    </note>
        /// </devdoc>
        [TypeConverterAttribute("System.ComponentModel.GuidConverter, " + AssemblyRef.System)]
        public Guid Category
        {
            get
            {
                return category;
            }
            set
            {
                category = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.EncryptionRequired"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the queue only accepts private
        ///       (encrypted) messages.</para>
        /// </devdoc>
        [DefaultValue(EncryptionRequired.Optional)]
        public EncryptionRequired EncryptionRequired
        {
            get
            {
                return encryptionRequired;
            }
            set
            {
                if (!ValidationUtility.ValidateEncryptionRequired(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(EncryptionRequired));

                encryptionRequired = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Label"]/*' />
        /// <devdoc>
        ///    <para>Indicates a description of the queue.</para>
        /// </devdoc>
        [DefaultValue("")]
        public string Label
        {
            get
            {
                return label;
            }
            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                label = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.MaximumJournalSize"]/*' />
        /// <devdoc>
        ///    <para>Indicates the maximum size of the journal associated with the queue.</para>
        /// </devdoc>
        [TypeConverterAttribute(typeof(System.Messaging.Design.SizeConverter))]
        public long MaximumJournalSize
        {
            get
            {
                return maximumJournalSize;
            }
            set
            {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidMaxJournalSize));

                maximumJournalSize = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.MaximumQueueSize"]/*' />
        /// <devdoc>
        ///    <para> Indicates the the maximum size of the queue.</para>
        /// </devdoc>
        [TypeConverterAttribute(typeof(System.Messaging.Design.SizeConverter))]
        public long MaximumQueueSize
        {
            get
            {
                return maximumQueueSize;
            }
            set
            {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidMaxQueueSize));

                maximumQueueSize = value;
            }
        }


        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.MulticastAddress"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the IP multicast address associated with the queue.</para>
        /// </devdoc>
        [DefaultValue("")]
        public string MulticastAddress
        {
            get
            {
                if (!MessageQueue.Msmq3OrNewer) //this feature is unavailable on win2k
                    throw new PlatformNotSupportedException(Res.GetString(Res.PlatformNotSupported));
                return multicastAddress;
            }
            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (!MessageQueue.Msmq3OrNewer) //this feature is unavailable on win2k
                    throw new PlatformNotSupportedException(Res.GetString(Res.PlatformNotSupported));

                multicastAddress = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Path"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the
        ///       location of
        ///       the queue that
        ///       will
        ///       be referenced by this object. .</para>
        /// </devdoc>
        [Editor("System.Messaging.Design.QueuePathEditor", "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        DefaultValue(""),
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)]
        public string Path
        {
            get
            {
                return path;
            }
            set
            {
                if (!MessageQueue.ValidatePath(value, true))
                    throw new ArgumentException(Res.GetString(Res.PathSyntax));
                if (value == null)
                    throw new ArgumentNullException("value");

                this.path = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Permissions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public AccessControlList Permissions
        {
            get
            {
                return permissions;
            }
            set
            {
                permissions = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Transactional"]/*' />
        /// <devdoc>
        ///     If a queue is transactional, it can only accept messages that are sent as part
        ///     of a transaction. However, messages can be retrieved from a local transaction
        ///     queue with or without using a transaction.
        /// </devdoc>
        [DefaultValue(false)]
        public bool Transactional
        {
            get
            {
                return transactional;
            }
            set
            {
                transactional = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.UninstallAction"]/*' />
        /// <devdoc>
        ///    <para>Indicates what the installer does with the queue at uninstall time: remove it, restore it
        ///       to its pre-installation state, or leave it in its current installed state.</para>
        /// </devdoc>
        [DefaultValue(UninstallAction.Remove)]
        public UninstallAction UninstallAction
        {
            get
            {
                return uninstallAction;
            }

            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1803:AvoidCostlyCallsWherePossible")]
            set
            {
                if (!Enum.IsDefined(typeof(UninstallAction), value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(UninstallAction));

                uninstallAction = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.UseJournalQueue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether messages retrieved from the queue are also copied to the
        ///       associated journal queue.</para>
        /// </devdoc>
        [DefaultValue(false)]
        public bool UseJournalQueue
        {
            get
            {
                return useJournalQueue;
            }
            set
            {
                useJournalQueue = value;
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Commit"]/*' />
        /// <devdoc>
        /// <para>Completes the installation process by committing <see cref='System.Messaging.MessageQueue'/> 
        /// installation information that was written to the registry by the <see cref='System.Messaging.MessageQueueInstaller.Install'/>
        /// method. This method is meant to be used by installation tools, which
        /// process the appropriate methods automatically.</para>
        /// </devdoc>
        public override void Commit(IDictionary savedState)
        {
            base.Commit(savedState);

            Context.LogMessage(Res.GetString(Res.ClearingQueue, Path));

            // make sure the queue is empty
            // we don't do this in Install because it can't be undone.
            MessageQueue queue = new MessageQueue(path);
            queue.Purge();
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        /// <para>Copies the property values of a <see cref='System.Messaging.MessageQueue'/> 
        /// component to this <see cref='System.Messaging.MessageQueueInstaller'/>
        /// . </para>
        /// </devdoc>
        public override void CopyFromComponent(IComponent component)
        {
            InternalCopyFromComponent(component);
        }


        private void InternalCopyFromComponent(IComponent component)
        {
            MessageQueue queue = component as MessageQueue;

            if (queue == null)
                throw new ArgumentException(Res.GetString(Res.NotAMessageQueue));

            if (queue.Path != null && queue.Path != string.Empty)
                Path = queue.Path;
            else
                throw new ArgumentException(Res.GetString(Res.IncompleteMQ));
        }


        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>Writes message queue information to the registry. This method is meant to be 
        ///       used by installation tools, which process the appropriate methods
        ///       automatically</para>
        /// </devdoc>
        public override void Install(IDictionary stateSaver)
        {
            base.Install(stateSaver);

            Context.LogMessage(Res.GetString(Res.CreatingQueue, Path));

            bool exists = MessageQueue.Exists(path);
            stateSaver["Exists"] = exists;
            MessageQueue queue = null;
            if (!exists)
                queue = MessageQueue.Create(Path, Transactional);
            else
            {
                // it exists. If it's got the right transactional property, we're OK.
                // Otherwise we have to recreate.
                queue = new MessageQueue(Path);

                // save off the properties for rollback
                stateSaver["Authenticate"] = queue.Authenticate;
                stateSaver["BasePriority"] = queue.BasePriority;
                stateSaver["Category"] = queue.Category;
                stateSaver["EncryptionRequired"] = queue.EncryptionRequired;
                stateSaver["Label"] = queue.Label;
                stateSaver["MaximumJournalSize"] = queue.MaximumJournalSize;
                stateSaver["MaximumQueueSize"] = queue.MaximumQueueSize;
                stateSaver["Path"] = queue.Path;
                stateSaver["Transactional"] = queue.Transactional;
                stateSaver["UseJournalQueue"] = queue.UseJournalQueue;
                if (MessageQueue.Msmq3OrNewer) //this feature is unavailable on win2k
                    stateSaver["MulticastAddress"] = queue.MulticastAddress;


                if (queue.Transactional != Transactional)
                {
                    // Messages won't be kept.
                    MessageQueue.Delete(Path);
                    queue = MessageQueue.Create(Path, Transactional);
                }
            }

            // now change all the properties to how we want them.
            queue.Authenticate = Authenticate;
            queue.BasePriority = BasePriority;
            queue.Category = Category;
            queue.EncryptionRequired = EncryptionRequired;
            queue.Label = Label;
            queue.MaximumJournalSize = MaximumJournalSize;
            queue.MaximumQueueSize = MaximumQueueSize;
            queue.UseJournalQueue = UseJournalQueue;
            if (MessageQueue.Msmq3OrNewer) //this feature is unavailable on win2k
                queue.MulticastAddress = MulticastAddress;


            if (permissions != null)
                queue.SetPermissions(permissions);
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.IsEquivalentInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsEquivalentInstaller(ComponentInstaller otherInstaller)
        {
            MessageQueueInstaller other = otherInstaller as MessageQueueInstaller;
            if (other == null)
                return false;

            return other.Path == Path;
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.RestoreQueue"]/*' />
        /// <devdoc>
        /// Called by Rollback and Uninstall to restore a queue to its state prior to Install
        /// </devdoc>
        private void RestoreQueue(IDictionary state)
        {
            bool exists = false;
            if (state != null && state["Exists"] != null)
                exists = (bool)state["Exists"];
            else
                // this can only happen at uninstall - the user might have deleted the .InstallState
                // file since Install ran. It's probably best to leave things the way they are.
                return;

            if (exists)
            {
                Context.LogMessage(Res.GetString(Res.RestoringQueue, Path));
                // the queue existed before install. Restore the properties

                MessageQueue queue = null;

                // first, restore the queue with the right Transactional property
                if (!MessageQueue.Exists(Path))
                {
                    // weird, but possible: the queue used to exist, but it doesn't now.
                    // put it back
                    queue = MessageQueue.Create(Path, (bool)state["Transactional"]);
                }
                else
                {
                    queue = new MessageQueue(Path);
                    if (queue.Transactional != (bool)state["Transactional"])
                    {
                        // the transactional property doesn't match. Recreate so it does
                        MessageQueue.Delete(Path);
                        queue = MessageQueue.Create(Path, (bool)state["Transactional"]);
                    }
                }

                // now change all the other properties to how they were.
                queue.Authenticate = (bool)state["Authenticate"];
                queue.BasePriority = (short)state["BasePriority"];
                queue.Category = (Guid)state["Category"];
                queue.EncryptionRequired = (EncryptionRequired)state["EncryptionRequired"];
                queue.Label = (string)state["Label"];
                queue.MaximumJournalSize = (long)state["MaximumJournalSize"];
                queue.MaximumQueueSize = (long)state["MaximumQueueSize"];
                if (MessageQueue.Msmq3OrNewer) //this feature is unavailable on win2k
                    queue.MulticastAddress = (string)state["MulticastAddress"];

                queue.UseJournalQueue = (bool)state["UseJournalQueue"];
                queue.ResetPermissions();
            }
            else
            {
                Context.LogMessage(Res.GetString(Res.RemovingQueue, Path));
                // it wasn't there before install, so let's make sure it still isn't
                if (MessageQueue.Exists(path))
                    MessageQueue.Delete(path);
            }
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para> Rolls back queue information that was written to the registry 
        ///       by the installation procedure. This method is meant to be used by installation
        ///       tools, which process the appropriate methods automatically.</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState)
        {
            base.Rollback(savedState);

            RestoreQueue(savedState);
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.ShouldSerializeCategory"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the value of the Category property should be persisted in 
        ///       generated code.</para>
        ///    <note type="rnotes">
        ///       The similar
        ///       "ShouldSerializeServicesDependedOn" in ServiceInstaller had dev comments that
        ///       indicated "persisted in code-gen". Is generated code the operative issue
        ///       here also?
        ///    </note>
        /// </devdoc>
        private bool ShouldSerializeCategory()
        {
            return !Category.Equals(Guid.Empty);
        }

        /// <include file='doc\MessageQueueInstaller.uex' path='docs/doc[@for="MessageQueueInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>Uninstalls the queue by removing information concerning it from the registry. 
        ///       If the <see cref='System.Messaging.MessageQueueInstaller.UninstallAction'/> is <see langword='Remove'/>,
        ///       Uninstall also deletes the queue associated with the <see cref='System.Messaging.MessageQueue'/>. </para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState)
        {
            base.Uninstall(savedState);

            if (UninstallAction == UninstallAction.Remove)
            {
                Context.LogMessage(Res.GetString(Res.DeletingQueue, Path));
                if (MessageQueue.Exists(Path))
                    MessageQueue.Delete(Path);
            }
        }
    }

}
