//------------------------------------------------------------------------------
// <copyright file="MessageBoxButtons.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum MessageBoxButtons {
        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.OK"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains an OK button. This field is
        ///       constant.
        ///    </para>
        /// </devdoc>
        OK               = 0x00000000,

        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.OKCancel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains OK and Cancel buttons. This field
        ///       is
        ///       constant.
        ///    </para>
        /// </devdoc>
        OKCancel         = 0x00000001,

        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.AbortRetryIgnore"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains Abort, Retry, and Ignore buttons.
        ///       This field is
        ///       constant.
        ///    </para>
        /// </devdoc>
        AbortRetryIgnore = 0x00000002,

        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.YesNoCancel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains Yes, No, and Cancel buttons. This
        ///       field is
        ///       constant.
        ///    </para>
        /// </devdoc>
        YesNoCancel      = 0x00000003,

        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.YesNo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains Yes and No buttons. This field is
        ///       constant.
        ///    </para>
        /// </devdoc>
        YesNo            = 0x00000004,

        /// <include file='doc\MessageBoxButtons.uex' path='docs/doc[@for="MessageBoxButtons.RetryCancel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the
        ///       message box contains Retry and Cancel buttons. This field
        ///       is
        ///       constant.
        ///    </para>
        /// </devdoc>
        RetryCancel      = 0x00000005

    }
}

