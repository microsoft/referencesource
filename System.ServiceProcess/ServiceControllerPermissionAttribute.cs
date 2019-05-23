//------------------------------------------------------------------------------
// <copyright file="ServiceControllerPermissionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System.ComponentModel;
    using System.Security;
    using System.Security.Permissions;
    using System.Globalization;
    
    /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute"]/*' />
    [
    AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly | AttributeTargets.Event, AllowMultiple = true, Inherited = false ),
    Serializable()
    ]     
    public class ServiceControllerPermissionAttribute : CodeAccessSecurityAttribute {
        private string machineName;
        private string serviceName;
        private ServiceControllerPermissionAccess permissionAccess;
        
        /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute.ServiceControllerPermissionAttribute"]/*' />
        public ServiceControllerPermissionAttribute(SecurityAction action)
        : base(action) {
            this.machineName = ".";
            this.serviceName = "*";
            this.permissionAccess = ServiceControllerPermissionAccess.Browse;
        }        

        /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute.MachineName"]/*' />
        public string MachineName {
            get {
                return this.machineName;
            }
            
            set {
                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(Res.GetString(Res.BadMachineName, value));
                    
                this.machineName = value;                    
            }
        }
        
        /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute.PermissionAccess"]/*' />
        public ServiceControllerPermissionAccess PermissionAccess {
            get {
                return this.permissionAccess;
            }
            
            set {
                this.permissionAccess = value;
            }
        }   
        
        /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute.ServiceName"]/*' />
        public string ServiceName {
            get {
                return this.serviceName;
            }
            
            set {                                                                                                                                                                  
                if (value == null)
                    throw new ArgumentNullException("value");

                if (!ServiceController.ValidServiceName(value))
                   throw new ArgumentException(Res.GetString(Res.ServiceName, value, ServiceBase.MaxNameLength.ToString(CultureInfo.CurrentCulture)));                                                
                                    
                this.serviceName = value;                                    
            }
        }                         
              
        /// <include file='doc\ServiceControllerPermissionAttribute.uex' path='docs/doc[@for="ServiceControllerPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission() {      
            if (Unrestricted) 
                return new ServiceControllerPermission(PermissionState.Unrestricted);
            
            return new ServiceControllerPermission(this.PermissionAccess, this.MachineName, this.ServiceName);
      
        }
    }    
}

