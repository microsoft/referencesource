using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Text;
using System.Messaging.Interop;
using System.Messaging;

namespace System.Messaging {
    
    /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class Trustee {
        string name;
        string systemName;
        TrusteeType trusteeType;
    
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get { return name; }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                name = value; 
            }
        }
    
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.SystemName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string SystemName {
            get { return systemName; }
            set { systemName = value; }
        }

        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.TrusteeType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TrusteeType TrusteeType {
            get { return trusteeType; }
            set { 
                if (!ValidationUtility.ValidateTrusteeType(value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(TrusteeType));
                    
                trusteeType = value; 
            }
        }
    
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.Trustee"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Trustee() {            
        }
        
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.Trustee1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Trustee(string name) : this(name, null) { }
        
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.Trustee2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Trustee(string name, string systemName) : this(name, systemName, System.Messaging.TrusteeType.Unknown) { }
        
        /// <include file='doc\Trustee.uex' path='docs/doc[@for="Trustee.Trustee3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Trustee(string name, string systemName, TrusteeType trusteeType) {            
            this.Name = name;
            this.SystemName = systemName;
            this.TrusteeType = trusteeType;
        }
    }
}
