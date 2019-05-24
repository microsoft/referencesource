//------------------------------------------------------------------------------
// <copyright file="ServiceNameConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
  
namespace System.ServiceProcess.Design {
    using System.ServiceProcess;                                                    
    using System.ComponentModel;
    using System.Diagnostics;
    using System;            
    using System.ComponentModel.Design.Serialization;
    using System.Reflection;
    using System.Collections; 
    using System.Globalization;
    
    internal class ServiceNameConverter : TypeConverter {
        private StandardValuesCollection values;
        private string previousMachineName;
    
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                return text;
            }
            return base.ConvertFrom(context, culture, value);
        }
        
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {                    
            ServiceController owner = (context == null) ? null : context.Instance as ServiceController;
            string machineName = ".";
            if (owner != null) 
                machineName = owner.MachineName;                        
        
            if (values == null || machineName != previousMachineName) {                
                try {
                    ServiceController[] installedServices = ServiceController.GetServices(machineName);
                    string[] serviceNames = new string[installedServices.Length];
                    for (int i = 0; i < installedServices.Length; i++) {
                        serviceNames[i] = installedServices[i].ServiceName;
                    }
                            
                    values = new StandardValuesCollection(serviceNames);
                    previousMachineName = machineName;
                }
                catch {
                    //Do Nothing
                }
            }
            return values;
        }        

        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }

        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }   
}
