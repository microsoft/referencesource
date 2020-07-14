//------------------------------------------------------------------------------
// <copyright file="EncryptionAlgorithm.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;
    using System.Messaging.Interop;

    /// <include file='doc\EncryptionAlgorithm.uex' path='docs/doc[@for="EncryptionAlgorithm"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the encryption algorithm used to encrypt the message body of a
    ///       private message.
    ///       
    ///    </para>
    /// </devdoc>
    public enum EncryptionAlgorithm
    {

        /// <include file='doc\EncryptionAlgorithm.uex' path='docs/doc[@for="EncryptionAlgorithm.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No encryption.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\EncryptionAlgorithm.uex' path='docs/doc[@for="EncryptionAlgorithm.Rc2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The value MQMSG_CALG_RC2. This is the default value for
        ///       the <see langword='EncryptAlgorithm'/> property of the Message Queuing
        ///       application's <see langword='MSMQMessage '/>
        ///       
        ///       
        ///       object.
        ///       
        ///    </para>
        /// </devdoc>
        Rc2 = NativeMethods.CALG_RC2,

        /// <include file='doc\EncryptionAlgorithm.uex' path='docs/doc[@for="EncryptionAlgorithm.Rc4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The value MQMSG_CALG_RC4. This corresponds to the less
        ///       secure option for the <see langword='EncryptAlgorithm '/>property of the
        ///       Message Queuing application's <see langword='MSMQMessage '/>
        ///       
        ///       object.
        ///       
        ///    </para>
        /// </devdoc>
        Rc4 = NativeMethods.CALG_RC4,
    }
}
