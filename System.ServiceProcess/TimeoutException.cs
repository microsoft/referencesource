//------------------------------------------------------------------------------
// <copyright file="TimeoutException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
    using System;        
    using System.Runtime.Serialization;
    
    [Serializable]
    public class TimeoutException : SystemException {
        public TimeoutException() : base() {
            HResult = HResults.ServiceControllerTimeout;
        }

        public TimeoutException(string message) : base(message) {
            HResult = HResults.ServiceControllerTimeout;
        }

        public TimeoutException(String message, Exception innerException)
            : base(message, innerException) {
            HResult = HResults.ServiceControllerTimeout;            
        }
     
        protected TimeoutException(SerializationInfo info, StreamingContext context) : base (info, context) {            
        }
    }        
}
