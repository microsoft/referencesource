//----------------------------------------------------
// <copyright file="ServiceControllerPermissionEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System.ComponentModel;
    using System.Security.Permissions;
    using System.Globalization;
    
    /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry"]/*' />
    [
    Serializable()
    ] 
    public class ServiceControllerPermissionEntry {
        private string machineName;
        private string serviceName;
        private ServiceControllerPermissionAccess permissionAccess;

        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.ServiceControllerPermissionEntry"]/*' />               
        public ServiceControllerPermissionEntry() {
            this.machineName = ".";
            this.serviceName = "*";
            this.permissionAccess = ServiceControllerPermissionAccess.Browse;
        }
        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.ServiceControllerPermissionEntry1"]/*' />                
        public ServiceControllerPermissionEntry(ServiceControllerPermissionAccess permissionAccess, string machineName, string serviceName) {
            if (serviceName == null)
                throw new ArgumentNullException("serviceName");

            if (!ServiceController.ValidServiceName(serviceName))
               throw new ArgumentException(Res.GetString(Res.ServiceName, serviceName, ServiceBase.MaxNameLength.ToString(CultureInfo.CurrentCulture)));                               
            
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.BadMachineName, machineName));
                                                          
            this.permissionAccess = permissionAccess;
            this.machineName = machineName;
            this.serviceName = serviceName;        
        }  
        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.ServiceControllerPermissionEntry2"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal ServiceControllerPermissionEntry(ResourcePermissionBaseEntry baseEntry) {
            this.permissionAccess = (ServiceControllerPermissionAccess)baseEntry.PermissionAccess;
            this.machineName = baseEntry.PermissionAccessPath[0]; 
            this.serviceName = baseEntry.PermissionAccessPath[1];  
        }

        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.MachineName"]/*' />                
        public string MachineName {
            get {
                return this.machineName;
            }            
        }
        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.PermissionAccess"]/*' />                
        public ServiceControllerPermissionAccess PermissionAccess {
            get {
                return this.permissionAccess;
            }            
        }   
        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.ServiceName"]/*' />                
        public string ServiceName {
            get {
                return this.serviceName;
            }                        
        }                         
        
        /// <include file='doc\ServiceControllerPermissionEntry.uex' path='docs/doc[@for="ServiceControllerPermissionEntry.GetBaseEntry"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal ResourcePermissionBaseEntry GetBaseEntry() {
            ResourcePermissionBaseEntry baseEntry = new ResourcePermissionBaseEntry((int)this.PermissionAccess, new string[] {this.MachineName, this.ServiceName});            
            return baseEntry;
        }         
    }        
}

