//------------------------------------------------------------------------------
// <copyright file="InstallException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration.Install {

    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\InstallException.uex' path='docs/doc[@for="InstallException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class InstallException : SystemException {
    
        /// <include file='doc\InstallException.uex' path='docs/doc[@for="InstallException.InstallException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstallException() : base() {
            HResult = HResults.Install;
        }

        /// <include file='doc\InstallException.uex' path='docs/doc[@for="InstallException.InstallException1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstallException(string message) : base(message) {
        }

        /// <include file='doc\InstallException.uex' path='docs/doc[@for="InstallException.InstallException2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstallException(string message, Exception innerException) : base(message, innerException) {
        }
        
        /// <include file='doc\InstallException.uex' path='docs/doc[@for="InstallException.InstallException3"]/*' />
        /// <internalonly/>
        protected InstallException(SerializationInfo info, StreamingContext context) : base (info, context) {            
        }
    }
}
