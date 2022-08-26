//------------------------------------------------------------------------------
// <copyright file="HashAlgorithm.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;
    using System.Messaging.Interop;

    /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the hash algorithm used by Message
    ///       Queuing when authenticating messages.
    ///       
    ///    </para>
    /// </devdoc>
    public enum HashAlgorithm
    {

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No hashing
        ///       algorithm.
        ///       
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Md2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       MD2 hashing algorithm.
        ///    </para>
        /// </devdoc>
        Md2 = NativeMethods.CALG_MD2,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Md4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       MD4 hashing algorithm.
        ///       
        ///    </para>
        /// </devdoc>
        Md4 = NativeMethods.CALG_MD4,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Md5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       MD5 hashing algorithm.
        ///    </para>
        /// </devdoc>
        Md5 = NativeMethods.CALG_MD5,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Sha"]/*' />
        /// <devdoc>
        ///    <para>
        ///       SHA hashing algorithm.
        ///    </para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        Sha = NativeMethods.CALG_SHA,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Mac"]/*' />
        /// <devdoc>
        ///    <para>
        ///       MAC keyed hashing algorithm.
        ///    </para>
        /// </devdoc>
        Mac = NativeMethods.CALG_MAC,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Sha256"]/*' />
        /// <devdoc>
        ///    <para>
        ///       SHA256 hashing algorithm.
        ///    </para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        Sha256 = NativeMethods.CALG_SHA256,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Sha384"]/*' />
        /// <devdoc>
        ///    <para>
        ///       SHA384 hashing algorithm.
        ///    </para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        Sha384 = NativeMethods.CALG_SHA384,

        /// <include file='doc\HashAlgorithm.uex' path='docs/doc[@for="HashAlgorithm.Sha512"]/*' />
        /// <devdoc>
        ///    <para>
        ///       SHA512 hashing algorithm.
        ///    </para>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        Sha512 = NativeMethods.CALG_SHA512,
    }
}
