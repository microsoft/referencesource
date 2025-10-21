//------------------------------------------------------------------------------
// <copyright file="GetChildAtPointSkip.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Windows.Forms;
    using System.ComponentModel;

    /// <include file='doc\GetChildAtPointSkip.uex' path='docs/doc[@for="GetChildAtPointSkip"]/*' />
    [
    Flags,
    ]
    [SuppressMessage("Microsoft.Naming", "CA1714:FlagsEnumsShouldHavePluralNames")]     // PM reviewed the enum name
    public enum GetChildAtPointSkip {
        /// <include file='doc\GetChildAtPointSkip.uex' path='docs/doc[@for="GetChildAtPointSkip.None"]/*' />
        None = 0x0000,
        /// <include file='doc\GetChildAtPointSkip.uex' path='docs/doc[@for="GetChildAtPointSkip.Invisible"]/*' />
        Invisible = 0x0001,         
        /// <include file='doc\GetChildAtPointSkip.uex' path='docs/doc[@for="GetChildAtPointSkip.Disabled"]/*' />
        Disabled = 0x0002,   
        /// <include file='doc\GetChildAtPointSkip.uex' path='docs/doc[@for="GetChildAtPointSkip.Transparent"]/*' />
        Transparent = 0x0004
    }
}

