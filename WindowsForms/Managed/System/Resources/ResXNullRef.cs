//------------------------------------------------------------------------------
// <copyright file="ResXNullRef.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if SYSTEM_WEB  // See DevDiv 9030
namespace System.PrivateResources {
#else
namespace System.Resources {
#endif

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;
    using System.ComponentModel;
    using System.Collections;
    using System.Resources;
    using System.Globalization;

    /// <include file='doc\ResXNullRef.uex' path='docs/doc[@for="ResXNullRef"]/*' />
    /// <devdoc>
    ///     ResX Null Reference class.  This class allows ResX to store null values.
    ///     It is a placeholder that is written into the file.  On read, it is replaced
    ///     with null.
    /// </devdoc>
    [Serializable]
    internal sealed class ResXNullRef {
    }
}

