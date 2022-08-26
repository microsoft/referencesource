//------------------------------------------------------------------------------
// <copyright file="MessageBoxDefaultButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System.Diagnostics.CodeAnalysis;

/*
 */
namespace System.Windows.Forms {

    /// <include file='doc\MessageBoxDefaultButton.uex' path='docs/doc[@for="MessageBoxDefaultButton"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>    
    [SuppressMessage("Microsoft.Design", "CA1027:MarkEnumsWithFlags")]
    public enum MessageBoxDefaultButton {
        /// <include file='doc\MessageBoxDefaultButton.uex' path='docs/doc[@for="MessageBoxDefaultButton.Button1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the first
        ///       button on the message box should be the default button. 
        ///    </para>
        /// </devdoc>
        Button1       = 0x00000000,
        /// <include file='doc\MessageBoxDefaultButton.uex' path='docs/doc[@for="MessageBoxDefaultButton.Button2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the second
        ///       button on the message box should be the default button. 
        ///    </para>
        /// </devdoc>
        Button2       = 0x00000100,

        /// <include file='doc\MessageBoxDefaultButton.uex' path='docs/doc[@for="MessageBoxDefaultButton.Button3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the third
        ///       button on the message box should be the default button. 
        ///    </para>
        /// </devdoc>
        Button3       = 0x00000200,
    }
}

