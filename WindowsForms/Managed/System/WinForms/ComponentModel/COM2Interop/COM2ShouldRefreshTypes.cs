//------------------------------------------------------------------------------
// <copyright file="COM2ShouldRefreshTypes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.ComponentModel;

    using System.Diagnostics;

    internal enum Com2ShouldRefreshTypes{
        Attributes,
        DisplayName,
        ReadOnly,
        TypeConverter,
        TypeEditor
    }
}
