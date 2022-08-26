//----------------------------------------------------
// <copyright file="MessageQueuePermissionEntryCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Collections;

    /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection"]/*' />        
    [Serializable()]
    public class MessageQueuePermissionEntryCollection : CollectionBase
    {
        MessageQueuePermission owner;

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.MessageQueuePermissionEntryCollection"]/*' />        
        /// <internalonly/>   
        internal MessageQueuePermissionEntryCollection(MessageQueuePermission owner)
        {
            this.owner = owner;
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.this"]/*' />        
        public MessageQueuePermissionEntry this[int index]
        {
            get
            {
                return (MessageQueuePermissionEntry)List[index];
            }
            set
            {
                List[index] = value;
            }

        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.Add"]/*' />        
        public int Add(MessageQueuePermissionEntry value)
        {
            return List.Add(value);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.AddRange"]/*' />        
        public void AddRange(MessageQueuePermissionEntry[] value)
        {
            if (value == null)
            {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = (i + 1))
            {
                this.Add(value[i]);
            }
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.AddRange1"]/*' />        
        public void AddRange(MessageQueuePermissionEntryCollection value)
        {
            if (value == null)
            {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = (i + 1))
            {
                this.Add(value[i]);
            }
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.Contains"]/*' />        
        public bool Contains(MessageQueuePermissionEntry value)
        {
            return List.Contains(value);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.CopyTo"]/*' />        
        public void CopyTo(MessageQueuePermissionEntry[] array, int index)
        {
            List.CopyTo(array, index);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.IndexOf"]/*' />        
        public int IndexOf(MessageQueuePermissionEntry value)
        {
            return List.IndexOf(value);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.Insert"]/*' />        
        public void Insert(int index, MessageQueuePermissionEntry value)
        {
            List.Insert(index, value);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.Remove"]/*' />        
        public void Remove(MessageQueuePermissionEntry value)
        {
            List.Remove(value);
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.OnClear"]/*' />        
        /// <internalonly/>                          
        protected override void OnClear()
        {
            this.owner.Clear();
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.OnInsert"]/*' />        
        /// <internalonly/>                          
        protected override void OnInsert(int index, object value)
        {
            this.owner.Clear();
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.OnRemove"]/*' />
        /// <internalonly/>                          
        protected override void OnRemove(int index, object value)
        {
            this.owner.Clear();
        }

        /// <include file='doc\MessageQueuePermissionEntryCollection.uex' path='docs/doc[@for="MessageQueuePermissionEntryCollection.OnSet"]/*' />
        /// <internalonly/>                          
        protected override void OnSet(int index, object oldValue, object newValue)
        {
            this.owner.Clear();
        }
    }
}

