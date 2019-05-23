//----------------------------------------------------
// <copyright file="ServiceControllerPermissionEntryCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System.Security.Permissions;
    using System.Collections;

    /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection"]/*' />
    [
    Serializable()
    ]
    public class ServiceControllerPermissionEntryCollection : CollectionBase {
        ServiceControllerPermission owner;
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.ServiceControllerPermissionEntryCollection"]/*' />        
        ///<internalonly/>   
        internal ServiceControllerPermissionEntryCollection(ServiceControllerPermission owner, ResourcePermissionBaseEntry[] entries) {
            this.owner = owner;
            for (int index = 0; index < entries.Length; ++index)
                this.InnerList.Add(new ServiceControllerPermissionEntry(entries[index]));
        }                                                                                                             
                                                                                                            
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.this"]/*' />
        public ServiceControllerPermissionEntry this[int index] {
            get {
                return (ServiceControllerPermissionEntry)List[index];
            }
            set {
                List[index] = value;
            }
            
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.Add"]/*' />        
        public int Add(ServiceControllerPermissionEntry value) {   
            return List.Add(value);
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.AddRange"]/*' />        
        public void AddRange(ServiceControllerPermissionEntry[] value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
    
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.AddRange1"]/*' />        
        public void AddRange(ServiceControllerPermissionEntryCollection value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }         
    
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.Contains"]/*' />        
        public bool Contains(ServiceControllerPermissionEntry value) {            
            return List.Contains(value);
        }
    
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.CopyTo"]/*' />        
        public void CopyTo(ServiceControllerPermissionEntry[] array, int index) {            
            List.CopyTo(array, index);
        }
    
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.IndexOf"]/*' />        
        public int IndexOf(ServiceControllerPermissionEntry value) {            
            return List.IndexOf(value);
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.Insert"]/*' />        
        public void Insert(int index, ServiceControllerPermissionEntry value) {            
            List.Insert(index, value);
        }
                
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.Remove"]/*' />        
        public void Remove(ServiceControllerPermissionEntry value) {
            List.Remove(value);                     
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.OnClear"]/*' />        
        ///<internalonly/>                          
        protected override void OnClear() {   
            this.owner.Clear();         
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.OnInsert"]/*' />        
        ///<internalonly/>                          
        protected override void OnInsert(int index, object value) {        
            this.owner.AddPermissionAccess((ServiceControllerPermissionEntry)value);
        }
        
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.OnRemove"]/*' />
        ///<internalonly/>                          
        protected override void OnRemove(int index, object value) {
            this.owner.RemovePermissionAccess((ServiceControllerPermissionEntry)value);
        }
                 
        /// <include file='doc\ServiceControllerPermissionEntryCollection.uex' path='docs/doc[@for="ServiceControllerPermissionEntryCollection.OnSet"]/*' />
        ///<internalonly/>                          
        protected override void OnSet(int index, object oldValue, object newValue) {     
            this.owner.RemovePermissionAccess((ServiceControllerPermissionEntry)oldValue);
            this.owner.AddPermissionAccess((ServiceControllerPermissionEntry)newValue);       
        }
    }
}   

