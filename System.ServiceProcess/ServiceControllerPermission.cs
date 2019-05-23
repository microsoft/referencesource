//------------------------------------------------------------------------------
// <copyright file="ServiceControllerPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System;    
    using System.Security.Permissions;
                                                                        
    /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Serializable()
    ]
    public sealed class ServiceControllerPermission : ResourcePermissionBase {      
         private ServiceControllerPermissionEntryCollection innerCollection;
        
        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.ServiceControllerPermission"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceControllerPermission() {
            SetNames();
        }                                                                
        
        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.ServiceControllerPermission1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceControllerPermission(PermissionState state) 
        : base(state) {
            SetNames();
        }
        
        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.ServiceControllerPermission2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceControllerPermission(ServiceControllerPermissionAccess permissionAccess, string machineName, string serviceName) {            
            SetNames();
            this.AddPermissionAccess(new ServiceControllerPermissionEntry(permissionAccess, machineName, serviceName));              
        }         
         
        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.ServiceControllerPermission3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ServiceControllerPermission(ServiceControllerPermissionEntry[] permissionAccessEntries) {            
            if (permissionAccessEntries == null)
                throw new ArgumentNullException("permissionAccessEntries");
                
            SetNames();            
            for (int index = 0; index < permissionAccessEntries.Length; ++index)
                this.AddPermissionAccess(permissionAccessEntries[index]);                          
        }

        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.PermissionEntries"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>                
        public ServiceControllerPermissionEntryCollection PermissionEntries {
            get {
                if (this.innerCollection == null)                     
                    this.innerCollection = new ServiceControllerPermissionEntryCollection(this, base.GetPermissionEntries()); 
                                                                           
                return this.innerCollection;                                                               
            }
        }

        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.AddPermissionAccess"]/*' />                
        ///<internalonly/> 
        internal void AddPermissionAccess(ServiceControllerPermissionEntry entry) {
            base.AddPermissionAccess(entry.GetBaseEntry());
        }
        
        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.Clear"]/*' />                        
        ///<internalonly/> 
        internal new void Clear() {
            base.Clear();
        }

        /// <include file='doc\ServiceControllerPermission.uex' path='docs/doc[@for="ServiceControllerPermission.RemovePermissionAccess"]/*' />                                                  
        ///<internalonly/> 
        internal void RemovePermissionAccess(ServiceControllerPermissionEntry entry) {
            base.RemovePermissionAccess(entry.GetBaseEntry());
        }                         
        
        private void SetNames() {
            this.PermissionAccessType = typeof(ServiceControllerPermissionAccess);            
            this.TagNames = new string[]{"Machine", "Service"};
        }    
    }
}       
