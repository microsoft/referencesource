//------------------------------------------------------------------------------
// <copyright file="ImageCodecFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {
    using System.Text;

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * Color channel flag constants
     */
    /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags()]
    public enum ImageCodecFlags {
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.Encoder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Encoder         = 0x00000001,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.Decoder"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Decoder         = 0x00000002,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.SupportBitmap"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SupportBitmap   = 0x00000004,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.SupportVector"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SupportVector   = 0x00000008,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.SeekableEncode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        SeekableEncode  = 0x00000010,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.BlockingDecode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        BlockingDecode  = 0x00000020,
        
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.Builtin"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Builtin         = 0x00010000,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.System"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        System          = 0x00020000,
        /// <include file='doc\ImageCodecFlags.uex' path='docs/doc[@for="ImageCodecFlags.User"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        User            = 0x00040000
    }
}
